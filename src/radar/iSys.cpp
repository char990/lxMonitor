#include <cstring>
#include <unistd.h>
#include <memory>

#include <radar/iSys.h>

#include <module/MyDbg.h>
#include <module/Epoll.h>
#include <module/Utils.h>
#include <module/ptcpp.h>
#include <gpio/GpioOut.h>

using namespace Utils;

using namespace Radar;
using namespace Radar::iSys;

iSys400xPower *iSys400xPwr;

#define RangeCM_sp_us(sp, us) (sp * us / 36000)
#define Timeval2us(tv) ((int64_t)1000000 * tv.tv_sec + tv.tv_usec)

#define TMR_RANGE 3000
#define TMR_SPECULATION 1000
#if TMR_RANGE <= TMR_SPECULATION
#error TMR_RANGE should greater than TMR_SPECULATION
#endif

/*****************************
Radar is iSYS 4001
******************************
SD: start delimiter to differ type of frame
1 = frame without data (SD1 = 0x10)
2 = frame with variable data length (SD2 = 0x68)
3 = frame with fixed data length (SD3 = 0xA2)
LE: length of the net data (data incl. DA, SA & FC appropriate byte 4 to byte n+7)
LEr: repetition of the net data length
DA: destination address
SA: source address
FC: function code
PDU: protocol data unit
FCS: frame checksum (addition of Bytes from Byte 4 to Byte n+7)
ED: end delimiter (ED = 0x16)

------------------------------------------------------------------------------
frame without data:
SD1	DA	SA	FC	FCS	ED
0	1	2	3	4	5
------------------------------------------------------------------------------

------------------------------------------------------------------------------
frame with variable data length:
SD2		LE		LEr		SD2		DA		SA		FC		PDU		FCS		ED
0		1		2		3		4		5		6		n+7		n+8		n+9
------------------------------------------------------------------------------

------------------------------------------------------------------------------
frame with fixed data length:
SD3	DA	SA	FC	PDU	FCS	ED
0	1	2	3	n+4	n+5	n+6
------------------------------------------------------------------------------

*****************************/

#define iSYS_ERROR_CNT 30

#define ISYS_SLAVE_ADDR 0x80  // slave addr
#define ISYS_MASTER_ADDR 0x01 // master addr

// protocol control chars
#define ISYS_FRM_CTRL_SD1 0x10 // without data
#define ISYS_FRM_CTRL_SD2 0x68 // variable length
#define ISYS_FRM_CTRL_SD3 0xA2 // fixed length
#define ISYS_FRM_CTRL_ED 0x16  // end delimiter

// position/offset
#define ISYS_FRM_SD21 0
#define ISYS_FRM_LE 1
#define ISYS_FRM_LER 2
#define ISYS_FRM_SD22 3
#define ISYS_FRM_DA 4
#define ISYS_FRM_SA 5
#define ISYS_FRM_FC 6
#define ISYS_FRM_PDU 7

// only for target list read
#define ISYS_FRM_OUT 7 // output channel
#define ISYS_FRM_TGT 8 // target list number

// command
#define RADAR_CMD_RD_DEV_NAME 0xd0
#define RADAR_CMD_START 0xd1
#define RADAR_CMD_STOP 0xd1
#define RADAR_CMD_RD_TGT_LIST 0xda
#define RADAR_CMD_RD_APP_SETTING 0xd4
#define RADAR_CMD_WR_APP_SETTING 0xd5
#define RADAR_CMD_EEPROM 0xdf
#define RADAR_CMD_FAILED 0xfd

//#define RX_BUF_SIZE 4096

const uint8_t read_device_name[] = {
	RADAR_CMD_RD_DEV_NAME
	// 0xd0 command
};

const uint8_t start_acquisition[] = {
	RADAR_CMD_START, 0x00, 0x00
	// 0xd1 command
	// 0x00 0x00 start acquisition
};

const uint8_t stop_acquisition[] = {
	RADAR_CMD_STOP, 0x00, 0x01
	// 0xd1 command
	// 0x00 0x01 stop acquisition
};

const uint8_t read_target_list_16bit[] = {
	RADAR_CMD_RD_TGT_LIST, 1, 16
	// 0xda read target list
	// 1	Target list filtered with parameters from output 1
	// 16	Output of sint16_t variable types (16-Bit output)
};

void VehicleFilter::PushVehicle(Vehicle *v)
{
	if (!v->IsValid())
	{
		return;
	}
	memcpy(&items[0], &items[1], VF_SIZE * sizeof(Vehicle));
	memcpy(&items[VF_SIZE], v, sizeof(Vehicle));
	isColsing = false;
	for (int i = 0; i < VF_SIZE; i++)
	{
		int usec = items[i + 1].usec - items[i].usec;
		int cm = items[i + 1].range - items[i].range;
		if (items[i].usec == 0 || usec <= 0 || usec > 500000 || cm > 0)
		{
			return;
		}
		int r = RangeCM_sp_us(items[i].speed, usec);
		cm = (cm > r) ? (cm - r) : (r - cm);
		if (cm > cmErr)
		{
			return;
		}
	}
	isColsing = true;
};

void VehicleFilter::Reset()
{
	bzero(items, sizeof(items));
	isColsing = false;
}

void TargetList::Reset()
{
	flag = 0;
	cnt = 0;
	for (auto &v : vehicles)
	{
		v.Reset();
	}
	minRangeVehicle = nullptr;
	vfilter.Reset();
}

int TargetList::DecodeTargetFrame(uint8_t *packet, int packetLen)
{
	cnt = 0;
	gettimeofday(&pktTime, nullptr);
	int fc = (packet[0] == ISYS_FRM_CTRL_SD2 /* check SD2 Frame */) ? 6 /* variable length frames */
																	: 3 /* fixed length frames */;
	int output_number = packet[fc + 1];
	int nrOfTargets = packet[fc + 2];
	/* check for valid amount of targets */
	if (nrOfTargets == 0xff)
	{ // 0xff means clipping, A frame with clipping doesn’t have any targets
		return 0xFF;
	}
	else if (nrOfTargets > MAX_TARGETS)
	{
		return -1;
	}
	if ((nrOfTargets * 7 + fc + 3 + 2) != packetLen) // check len
	{
		return -1;
	}
	int64_t usec = Timeval2us(pktTime);
	auto pData = &packet[fc + 3];
	for (int i = 0; i < nrOfTargets; i++)
	{
		auto &v = vehicles[cnt];
		// time
		v.usec = usec;
		// Signal
		v.signal = *pData;
		pData += 1;
		// Velocity
		v.speed = Cnvt::GetS16hl(pData) * 3600 / 100000; // 0.01m/s -> km/h;
		pData += 2;
		// Range: 4004 & 6003 => -32768 … 32767 [mm]; others => -32768 … 32767 [cm]
		v.range = Cnvt::GetS16hl(pData);
		pData += 2;
		// angle
		v.angle = Cnvt::GetS16hl(pData) / 100; // -327.68 … 327.67 [°]
		pData += 2;
		if (v.signal >= uciradar.minSignal && v.speed >= uciradar.minSpeed && v.range >= uciradar.minRange)
		{
			cnt++;
		}
	}
	return cnt;
}

void TargetList::MakeTargetMsg(uint8_t *buf, int *len)
{
	uint8_t *ptr = buf;
	*ptr++ = ISYS_FRM_CTRL_SD2;
	int pdulen = 5 + cnt * 7;
	*ptr++ = pdulen;
	*ptr++ = pdulen;
	*ptr++ = ISYS_FRM_CTRL_SD2;
	*ptr++ = ISYS_MASTER_ADDR;
	*ptr++ = ISYS_SLAVE_ADDR;
	*ptr++ = RADAR_CMD_RD_TGT_LIST;
	*ptr++ = 1; // list number
	*ptr++ = cnt;
	for (int i = 0; i < cnt; i++)
	{
		auto &v = vehicles[i];
		*ptr++ = v.signal;
		ptr = Cnvt::PutU16(v.speed * 100000 / 3600, ptr);
		ptr = Cnvt::PutU16(v.range, ptr);
		ptr = Cnvt::PutU16(v.angle * 100, ptr);
	}
	// chekcsum
	ptr = buf + ISYS_FRM_DA;
	uint8_t c = 0;
	for (int i = 0; i < pdulen; i++)
	{
		c += *ptr++;
	}
	*ptr++ = c;
	*ptr = ISYS_FRM_CTRL_ED;
	*len = pdulen + 6;
}

int TargetList::SaveMeta(const char *comment, const char *details)
{
	csv.SaveRadarMeta(pktTime, comment, details);
	return 0;
}

int TargetList::SaveTarget(const char *comment)
{
	if (cnt == 0)
	{ // no vehicle
		switch (flag)
		{
		case -1:
			// csv.SaveRadarMeta(time, "Invalid taget list", nullptr);
			break;
		case 0:
			if (hasVehicle)
			{
				hasVehicle = false;
				// csv.SaveRadarMeta(time, NO_VEHICLE, nullptr);
			}
			break;
		case 0xFF:
			// csv.SaveRadarMeta(time, "Clipping", nullptr);
			break;
		}
	}
	else
	{
		char xbuf[1024];
		xbuf[0] = 0;
		hasVehicle = true;
		if (uciradar.radarMode == 0)
		{
			Print(xbuf);
		}
		else if (uciradar.radarMode == 1)
		{
			minRangeVehicle->Print(xbuf);
		}
		csv.SaveRadarMeta(pktTime, comment, xbuf);
	}
	return 0;
}

int TargetList::Print(char *buf)
{
	int len = 0; // sprintf(buf, "%d", cnt);
	for (int i = 0; i < cnt; i++)
	{
		if (i > 0)
		{
			buf[len++] = ',';
		}
		len += vehicles[i].Print(buf + len);
	}
	return len;
}

int TargetList::Print()
{
	char buf[1024];
	Print(buf);
	return printf("%s\n", buf);
}

void TargetList::Refresh()
{
	minRangeVehicle = &vehicles[0];
	for (int i = 1; i < cnt; i++)
	{
		auto &v = vehicles[i];
		if (v.range < minRangeVehicle->range)
		{
			minRangeVehicle = &vehicles[i];
		}
	}
	vfilter.PushVehicle(minRangeVehicle);
}

bool iSys400xPower::TaskRePower_(int *_ptLine)
{
	PT_BEGIN();
	while (true)
	{
		PT_WAIT_UNTIL(iSysPwr == PwrSt::AUTOPWR_OFF);
		PrintDbg(DBG_LOG, "iSys power-reset: START");
		RelayNcOff();
		tmrRePwr.Setms(ISYS_PWR_0_T);
		PT_WAIT_UNTIL(tmrRePwr.IsExpired());
		RelayNcOn();
		tmrRePwr.Setms(ISYS_PWR_1_T);
		PT_WAIT_UNTIL(tmrRePwr.IsExpired());
		iSysPwr = PwrSt::PWR_ON;
		PrintDbg(DBG_LOG, "iSys power-reset: DONE");
	};
	PT_END();
}

iSys400x::iSys400x(UciRadar &uciradar, std::vector<int> &distance)
	: IRadar(uciradar), targetlist(uciradar), distance(distance)
{
	oprSp = new OprSp(uciradar.radarPort, uciradar.radarBps, nullptr);
	radarStatus = RadarStatus::POWER_UP;
	tmrPwrDelay.Setms(0);
	Reset();
}

iSys400x::~iSys400x()
{
	delete oprSp;
}

void iSys400x::SendSd2(const uint8_t *p, int len)
{
	uint8_t *buf = new uint8_t[6 + len + 2];
#if 0
		// this piece of code makes system reboot
		buf[0] = ISYS_FRM_CTRL_SD2;
		buf[1] = len + 2;
		buf[2] = len + 2;
		buf[3] = ISYS_FRM_CTRL_SD2;
		buf[4] = ISYS_SLAVE_ADDR;
		buf[5] = ISYS_MASTER_ADDR;
		oprSp->Tx(buf, 6);
		oprSp->Tx(p, len);
		char c = buf[4] + buf[5];
		for (int i = 0; i < len; i++)
		{
			c += *p;
			p++;
		}
		buf[0] = c;
		buf[1] = ISYS_FRM_CTRL_ED;
		oprSp->Tx(buf, 2);
#else
	buf[0] = ISYS_FRM_CTRL_SD2;
	buf[1] = len + 2;
	buf[2] = len + 2;
	buf[3] = ISYS_FRM_CTRL_SD2;
	buf[4] = ISYS_SLAVE_ADDR;
	buf[5] = ISYS_MASTER_ADDR;
	memcpy(buf + 6, p, len);
	char c = buf[4] + buf[5];
	for (int i = 0; i < len; i++)
	{
		c += *p;
		p++;
	}
	buf[len + 6] = c;
	buf[len + 7] = ISYS_FRM_CTRL_ED;
	oprSp->Tx(buf, 6 + len + 2);
#endif
	delete[] buf;
}

bool iSys400x::VerifyCmdAck()
{
	auto len = ReadPacket();
	return (len != 0 && packet[ISYS_FRM_FC] == RADAR_CMD_START);
}

int iSys400x::ReadPacket()
{
	packetLen = 0;
	int len = oprSp->rxRingBuf->Cnt();
	if (len > 0)
	{
		auto buf = new uint8_t[len];
		oprSp->rxRingBuf->Pop(buf, len);
		int indexSD2 = -1;
		for (int i = 0; i < len; i++)
		{
			if (buf[i] == ISYS_FRM_CTRL_SD2 && indexSD2 < 0)
			{
				indexSD2 = i;
			}
			else if (buf[i] == ISYS_FRM_CTRL_ED && indexSD2 >= 0)
			{
				int rl = i - indexSD2 + 1;
				if (rl <= MAX_PACKET_SIZE && ChkRxFrame(buf + indexSD2, rl) == iSYS_Status::ISYS_RX_SUCCESS)
				{
					memcpy(packet, buf + indexSD2, rl);
					packetLen = rl;
				}
				indexSD2 = -1;
			}
		}
		delete[] buf;
	}
	return packetLen;
}

iSYS_Status iSys400x::ChkRxFrame(uint8_t *rxBuffer, int len)
{
	int chk = rxBuffer[ISYS_FRM_LE] + ISYS_FRM_DA;									   // position of FCS : frame checksum
	if (len < 9 || rxBuffer[ISYS_FRM_LE] != rxBuffer[ISYS_FRM_LER] || (chk + 2) != len // length
		|| rxBuffer[ISYS_FRM_SD22] != ISYS_FRM_CTRL_SD2)
	{
		return iSYS_Status::ISYS_RX_FRAME_ERROR;
	}
	if (rxBuffer[ISYS_FRM_DA] != ISYS_MASTER_ADDR || rxBuffer[ISYS_FRM_SA] != ISYS_SLAVE_ADDR)
	{
		return iSYS_Status::ISYS_RX_ADDR_ERROR;
	}
	uint8_t c = 0;
	for (int i = ISYS_FRM_DA; i < chk; i++)
	{
		c += rxBuffer[i];
	}
	return c == rxBuffer[chk] ? iSYS_Status::ISYS_RX_SUCCESS : iSYS_Status::ISYS_RX_SUM_ERROR;
}

int iSys400x::DecodeDeviceName()
{
	if (packet[ISYS_FRM_FC] != RADAR_CMD_RD_DEV_NAME)
	{
		return -1;
	}
	PrintDbg(DBG_LOG, "%s:%s", uciradar.name.c_str(), (char *)packet + ISYS_FRM_PDU);
	return 0;
}

void iSys400x::CmdReadDeviceName()
{
	SendSd2(read_device_name, countof(read_device_name));
}

void iSys400x::CmdStartAcquisition()
{
	SendSd2(start_acquisition, countof(start_acquisition));
}

void iSys400x::CmdStopAcquisition()
{
	SendSd2(stop_acquisition, countof(stop_acquisition));
}

void iSys400x::CmdReadTargetList()
{
	SendSd2(read_target_list_16bit, countof(read_target_list_16bit));
}

void iSys400x::Reset()
{
	TaskRangeReset();
	TaskRadarPoll_Reset();
}

void iSys400x::TaskRadarPoll_Reset()
{
	packetLen = 0;
	ClearRxBuf();
	taskRadarPoll_ = 0;
	targetlist.Reset();
	ReloadTmrssTimeout();
	isConnected = Utils::STATE3::S3_NA;
}


void iSys400x::TaskRangeReset()
{
	uciRangeIndex = 0;
	tmrRange.Setms(TMR_RANGE);
	tmrSpeculation.Clear();
	v1st.Reset();
	v1stClosing = false;
	v2nd.Reset();
	v2ndClosing = false;
}

bool iSys400x::TaskRadarPoll()
{
	if (iSys400xPwr->iSysPwr == iSys400xPower::PwrSt::PWR_ON)
	{
		TaskRadarPoll_(&taskRadarPoll_);
		if (ssTimeout.IsExpired())
		{
			if (tmrPwrDelay.IsExpired())
			{
				iSys400xPwr->AutoPwrOff();
				radarStatus = RadarStatus::NO_CONNECTION;
				if (!IsNotConnected())
				{
					Connected(false);
					PrintDbg(DBG_LOG, "%s NO_CONNECTION", uciradar.name.c_str());
				}
				if (pwrDelay == 0)
				{
					pwrDelay = 15 * 1000; // start from 15s
				}
				else if (pwrDelay < 64 * 60 * 1000) // to 64 minutes
				{
					pwrDelay *= 2;
				}
				tmrPwrDelay.Setms(pwrDelay);
			}
		}
		else
		{
			if (radarStatus == RadarStatus::EVENT)
			{
				if (!IsConnected())
				{
					Connected(true);
					PrintDbg(DBG_LOG, "%s CONNECTED", uciradar.name.c_str());
				}
				pwrDelay = 0;
				tmrPwrDelay.Setms(pwrDelay);
			}
		}
	}
	else
	{
		if (taskRadarPoll_ != 0)
		{
			Reset();
		}
	}
	return true;
}

bool iSys400x::TaskRadarPoll_(int *_ptLine)
{
	PT_BEGIN();
	while (true)
	{
		ReloadTmrssTimeout();
		// get device name
		radarStatus = RadarStatus::POWER_UP;
		do
		{
			ClearRxBuf();
			CmdReadDeviceName();
			tmrTaskRadar.Setms(100 - 1);
			PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
			if (ReadPacket() > 0)
			{
				if (DecodeDeviceName() == 0)
				{
					radarStatus = RadarStatus::EVENT;
					ReloadTmrssTimeout();
				}
			}
		} while (radarStatus != RadarStatus::EVENT);
		PT_YIELD();

		// stop then start
		radarStatus = RadarStatus::INITIALIZING;
		do
		{
			CmdStartAcquisition();
			tmrTaskRadar.Setms(100 - 1);
			PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
			if (!VerifyCmdAck())
			{
				CmdStopAcquisition();
				tmrTaskRadar.Setms(100 - 1);
				PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
			}
			else
			{
				radarStatus = RadarStatus::EVENT;
				ReloadTmrssTimeout();
			}
		} while (radarStatus != RadarStatus::EVENT);
		PT_YIELD();

		// read target list
		radarStatus = RadarStatus::READY;
		do
		{
			ClearRxBuf();
			CmdReadTargetList();
			tmrTaskRadar.Setms(100 - 1);
			PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
			if (ReadPacket() > 0)
			{
				int x = targetlist.DecodeTargetFrame(packet, packetLen);
				targetlist.flag = x;
				if (x >= 0)
				{
					if (x == 0 || x == 0xFF)
					{
						targetlist.minRangeVehicle = nullptr;
					}
					else
					{
						targetlist.Refresh();
					}
					radarStatus = RadarStatus::EVENT;
					ReloadTmrssTimeout();
				}
			}
		} while (true);
	};
	PT_END();
}

int iSys400x::SaveTarget(const char *comment)
{
	if (Vdebug() >= 3)
	{
		if (targetlist.minRangeVehicle != nullptr)
		{
			targetlist.minRangeVehicle->Print();
		}
	}
	return targetlist.SaveTarget(comment);
}

int iSys400x::SaveMeta(const char *comment, const char *details)
{
	if (Vdebug() >= 3)
	{
		PrintDbg(DBG_PRT, "\tMeta=%s,%s\n", comment, details);
	}
	return targetlist.SaveMeta(comment, details);
}

void iSys400x::ReloadTmrssTimeout()
{
	ssTimeout.Setms(ISYS_PWR_0_T + ISYS_PWR_1_T + 1000);
}

int iSys400x::CheckRange()
{
	auto v = targetlist.minRangeVehicle;
	int speculation = 0;
	int photo = 0;
	auto Speculate_v1st = [&]
	{
		if (v1st.IsValid() && v1stClosing && (uciRangeIndex < distance.size()) && !tmrSpeculation.IsExpired())
		{ // refresh v1st based on speculation
			if (tmrSpeculation.IsClear())
			{
				tmrSpeculation.Setms(TMR_SPECULATION);
			}
			struct timeval tv;
			gettimeofday(&tv, nullptr);
			int64_t usec = Timeval2us(tv);
			int us = usec - v1st.usec;
			if (us > 0)
			{
				v1st.usec = usec;
				v1st.range -= RangeCM_sp_us(v1st.speed, us);
				if (Vdebug() >= 2)
				{
					char buf[128];
					v1st.Print(buf);
					PrintDbg(DBG_PRT, "\tv1st-Speculation:%s", buf);
				}
				speculation = 1;
#if 1
				if (v1st.range <= distance.back())
				{
					photo = 1;
					uciRangeIndex = distance.size();
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tphoto = 1[1]");
					}
				}
#endif
			}
		}
	};

	if (v == nullptr)
	{
		if (tmrRange.IsExpired())
		{
			TaskRangeReset();
			return 0;
		}
		if (!v1st.IsValid())
		{
			return 0;
		}
		Speculate_v1st();
	}
	else
	{
		tmrRange.Setms(TMR_RANGE);
		tmrSpeculation.Clear();
		if (!v1st.IsValid())
		{
			memcpy(&v1st, v, sizeof(v1st));
			speculation = 0;
		}
		else
		{
			if (v2nd.IsValid())
			{ // with v2nd exists, isys lost v1st
				Speculate_v1st();
				memcpy(&v2nd, v, sizeof(v2nd));
			}
			else
			{ // no v2nd, there is only v1st
				if (v1st.range < uciradar.rangeLast && v->range > v1st.range + uciradar.rangeRise)
				{ // range suddenly changed, means new vehicle
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tuciRangeIndex=%d->0\tv1st.range=%d, v->range=%d",
								 uciRangeIndex, v1st.range, v->range);
					}
					if (uciRangeIndex == distance.size())
					{ // restart, this is v1st
						memcpy(&v1st, v, sizeof(v1st));
						speculation = 0;
					}
					else
					{ // this is v2nd
						Speculate_v1st();
						memcpy(&v2nd, v, sizeof(v2nd));
						v2ndClosing = false;
					}
					uciRangeIndex = 0;
				}
				else
				{
					memcpy(&v1st, v, sizeof(v1st));
					speculation = 0;
				}
			}
		}
	}
	#if 1
	if (!speculation)
	{ // v1st must be valid, so only check v1st
		if (targetlist.IsClosing())
		{
			v1stClosing = true;
			if (uciRangeIndex < distance.size())
			{
				if (v1st.range <= distance[uciRangeIndex])
				{
					photo = 1;
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tphoto = 1[2], uciRangeIndex=%d", uciRangeIndex);
					}
					while (uciRangeIndex < distance.size() && v1st.range <= distance[uciRangeIndex])
					{
						uciRangeIndex++;
					};
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tuciRangeIndex=%d", uciRangeIndex);
					}
				}
				else if (0) // Vdebug() >= 2)
				{
					PrintDbg(DBG_PRT, "\tFALSE 1: uciRangeIndex=%d, v1st.range=%d", uciRangeIndex, v1st.range);
				}
			}
		}
	}
	else
	{ // speculation is running & check v2nd as well
		if (v2nd.IsValid() && targetlist.IsClosing())
		{
			v2ndClosing = true;
			if (uciRangeIndex < distance.size())
			{
				if (v2nd.range <= distance[uciRangeIndex])
				{
					photo = 1;
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tphoto = 1[3]");
					}
					while (uciRangeIndex < distance.size() && v2nd.range <= distance[uciRangeIndex])
					{
						uciRangeIndex++;
					};
				}
				else if (0) // Vdebug() >= 2)
				{
					PrintDbg(DBG_PRT, "\tFALSE 2: uciRangeIndex=%d, v2nd.range=%d", uciRangeIndex, v2nd.range);
				}
			}
		}
	}
	#else
	// v1st must be valid
	if (!v2nd.IsValid())
	{ // v2nd not valid
		if (targetlist.IsClosing())
		{
			v1stClosing = true;
		}
		if(v1stClosing)
		{
			if (uciRangeIndex < distance.size())
			{
				if (v1st.range <= distance[uciRangeIndex])
				{
					photo = 1;
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tphoto = 1[2], uciRangeIndex=%d", uciRangeIndex);
					}
					while (uciRangeIndex < distance.size() && v1st.range <= distance[uciRangeIndex])
					{
						uciRangeIndex++;
					};
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tuciRangeIndex=%d", uciRangeIndex);
					}
				}
				else if (0) // Vdebug() >= 2)
				{
					PrintDbg(DBG_PRT, "\tFALSE 1: uciRangeIndex=%d, v1st.range=%d", uciRangeIndex, v1st.range);
				}
			}
		}
	}
	else
	{ // v2nd valid
		if (targetlist.IsClosing())
		{
			v2ndClosing = true;
		}
		if(v2ndClosing)
		{	
			if (uciRangeIndex < distance.size())
			{
				if (v2nd.range <= distance[uciRangeIndex])
				{
					photo = 1;
					if (Vdebug() >= 2)
					{
						PrintDbg(DBG_PRT, "\tphoto = 1[3]");
					}
					while (uciRangeIndex < distance.size() && v2nd.range <= distance[uciRangeIndex])
					{
						uciRangeIndex++;
					};
				}
				else if (0) // Vdebug() >= 2)
				{
					PrintDbg(DBG_PRT, "\tFALSE 2: uciRangeIndex=%d, v2nd.range=%d", uciRangeIndex, v2nd.range);
				}
			}
		}
	}
	#endif
	if (photo)
	{
		int r = 1 + speculation;
		if (v2nd.IsValid())
		{
			memcpy(&v1st, &v2nd, sizeof(v1st));
			v1stClosing = v2ndClosing;
			v2nd.Reset();
			v2ndClosing = false;
			tmrSpeculation.Clear();
			speculation = 0;
		}
		else if (speculation) // only v1st && speculation
		{
			TaskRangeReset();
		}
		return r;
	}
	return 0;
}

#if 0
#include <3rdparty/catch2/EnableTest.h>
#if _ENABLE_TEST_ == 1
#include <time.h>
#include <string>
#include <unistd.h>
#include <3rdparty/catch2/catch.hpp>

		TEST_CASE("Class iSys400x", "[iSys400x]")
		{
			uint8_t packet[MAX_PACKET_SIZE];
			int packetLen;
			std::string name = std::string("iSys400x");

			TargetList list1{name, 4001};
			list1.cnt = 1;
			list1.vehicles[0] = {60, 80, 15000, 12};

			TargetList list2{name, 4001};
			list2.cnt = 2;
			list2.vehicles[0] = {60, 80, 14000, 12};
			list2.vehicles[1] = {61, 70, 15000, 12};

			TargetList list3{name, 4001};
			list3.cnt = 3;
			list3.vehicles[0] = {60, 80, 13000, 12};
			list3.vehicles[1] = {61, 70, 14000, 12};
			list3.vehicles[2] = {62, 60, 15000, 12};

			TargetList target{name, 4001};

			SECTION("DecodeTargetFrame")
			{
				int x;
				list1.MakeTargetMsg(packet, &packetLen);
				x = target.DecodeTargetFrame(packet, packetLen);
				target.Print();
				REQUIRE(x == 1);
				list2.MakeTargetMsg(packet, &packetLen);
				x = target.DecodeTargetFrame(packet, packetLen);
				target.Print();
				REQUIRE(x == 2);
				list3.MakeTargetMsg(packet, &packetLen);
				x = target.DecodeTargetFrame(packet, packetLen);
				target.Print();
				REQUIRE(x == 3);
			}
		}

#endif
#endif

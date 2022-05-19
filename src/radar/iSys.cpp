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

iSys400xPower *isys400xpwr;
iSys400x *isys400x[2];

#define ISYS_TIMEOUT (ISYS_PWR_0_T + ISYS_PWR_1_T + 1000)

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
#define RADAR_CMD_RD_FREQ_CHN 0xd2
#define RADAR_CMD_SET_FREQ_CHN 0xd3
#define RADAR_CMD_RD_APP_SETTING 0xd4
#define RADAR_CMD_WR_APP_SETTING 0xd5
#define RADAR_CMD_EEPROM 0xdf
#define RADAR_CMD_FAILED 0xfd

// parameter setting code
#define CODE_RANGE_MIN 0x08
#define CODE_RANGE_MAX 0x09
#define CODE_SIGNAL_MIN 0x0A
#define CODE_SIGNAL_MAX 0x0B
#define CODE_SPEED_MIN 0x0C
#define CODE_SPEED_MAX 0x0D
#define CODE_SPEED_DIR 0x0E
#define CODE_FLT_SIGNAL 0x16
#define APP_SETTING_SIZE 8

const uint8_t APP_SETTING_LIST[APP_SETTING_SIZE] = {
	CODE_RANGE_MIN, CODE_RANGE_MAX, CODE_SIGNAL_MIN, CODE_SIGNAL_MAX,
	CODE_SPEED_MIN, CODE_SPEED_MAX, CODE_SPEED_DIR, CODE_FLT_SIGNAL};

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

const uint8_t read_FreqChannel[] = {
	RADAR_CMD_RD_FREQ_CHN, 0x00, 0x04
	// 0xd2 command
	// 0x00 0x04 FreqChannel
};

const uint8_t set_FreqChannel[] = {
	RADAR_CMD_SET_FREQ_CHN, 0x00, 0x04
	// 0xd3 command
	// 0x00 0x04 FreqChannel
};

void VehicleFilter::PushVehicle(Vehicle *v)
{
	if (!v->IsValid())
	{
		return;
	}
	memcpy(&items[0], &items[1], VF_SIZE * sizeof(Vehicle));
	memcpy(&items[VF_SIZE], v, sizeof(Vehicle));
	isCA = false;
	for (int i = 0; i < VF_SIZE; i++)
	{
		int speed = items[i].speed;
		int usec = items[i + 1].usec - items[i].usec;
		int cm = items[i + 1].range - items[i].range;
		if (items[i].usec == 0 || usec <= 0 || usec > 500000 || (speed > 0 && cm > 0) || (speed < 0 && cm < 0))
		{
			return;
		}
		if (speed < 0)
		{
			cm = -cm;
			speed = -speed;
		}
		int r = RangeCM_sp_us(speed, usec);
		cm = (cm > r) ? (cm - r) : (r - cm);
		if (cm > cmErr)
		{
			return;
		}
	}
	isCA = true;
	isSlowdown = false;
	isSpeedUp = false;

	if (items[0].speed > 0 && items[1].speed > 0 && items[2].speed > 0)
	{
		if (items[0].speed > items[1].speed && items[1].speed > items[2].speed > 0)
		{
			isSlowdown = true;
		}
		else if (items[0].speed < items[1].speed && items[1].speed<items[2].speed> 0)
		{
			isSpeedUp = true;
		}
	}
	else if (items[0].speed < 0 && items[1].speed < 0 && items[2].speed < 0)
	{
		if (items[0].speed > items[1].speed && items[1].speed > items[2].speed > 0)
		{
			isSpeedUp = true;
		}
		else if (items[0].speed < items[1].speed && items[1].speed<items[2].speed> 0)
		{
			isSlowdown = true;
		}
	}
};

void VehicleFilter::Reset()
{
	bzero(items, sizeof(items));
	isCA = false;
	isSlowdown = false;
}
#if 0
void TargetList::Reset()
{
	flag = 0;
	cnt = 0;
	for (auto &v : vClos)
	{
		v.Reset();
	}
	minRangeVehicle = nullptr;
	vfilter.Reset();
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
		auto &v = vClos[i];
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
		minRangeVehicle->Print(xbuf);
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
		len += vClos[i].Print(buf + len);
	}
	return len;
}

int TargetList::Print()
{
	char buf[1024];
	Print(buf);
	return printf("%s\n", buf);
}
#endif

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

iSys400x::iSys400x(UciRadar &uciradar)
	: IRadar(uciradar)
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
	delete[] buf;
}

bool iSys400x::VerifyCmdAck(uint8_t cmd)
{
	auto len = ReadPacket();
	return (len != 0 && packet[ISYS_FRM_FC] == cmd);
}

void iSys400x::CmdReadAppSetting(uint8_t index)
{
	uint8_t cmd[3];
	cmd[0] = RADAR_CMD_RD_APP_SETTING;
	cmd[1] = 0x01;
	cmd[2] = APP_SETTING_LIST[index];
	SendSd2(cmd, 3);
}

void iSys400x::CmdWriteAppSetting(uint8_t index, int16_t data)
{
	uint8_t cmd[5];
	cmd[0] = RADAR_CMD_WR_APP_SETTING;
	cmd[1] = 0x01;
	cmd[2] = APP_SETTING_LIST[index];
	cmd[3] = data / 0x100;
	cmd[4] = data;
	SendSd2(cmd, 5);
}

int iSys400x::DecodeAppSetting()
{
	int c = -1;
	if (VerifyCmdAck(RADAR_CMD_RD_APP_SETTING))
	{
		c = packet[ISYS_FRM_PDU] * 0x100 + packet[ISYS_FRM_PDU + 1];
	}
	return c;
}

void iSys400x::CmdSaveToEEprom()
{
	uint8_t cmd[2] = {RADAR_CMD_EEPROM, 0x04};
	SendSd2(cmd, 2);
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

void iSys400x::CmdReadFreqChannel()
{
	SendSd2(read_FreqChannel, countof(read_FreqChannel));
}

void iSys400x::CmdSetFreqChannel()
{
	int len = countof(set_FreqChannel);
	uint8_t *buf = new uint8_t[len + 2];
	memcpy(buf, set_FreqChannel, len);
	buf[len++] = 0;
	buf[len++] = uciradar.radarMode;
	SendSd2(buf, len);
	delete[] buf;
}

int iSys400x::DecodeFreqChannel()
{
	int c = -1;
	if (VerifyCmdAck(RADAR_CMD_RD_FREQ_CHN))
	{
		c = packet[ISYS_FRM_PDU] * 0x100 + packet[ISYS_FRM_PDU + 1];
		PrintDbg(DBG_LOG, "%s:freq channel:%d", uciradar.name.c_str(), c);
	}
	return c;
}

void iSys400x::CmdReadTargetList()
{
	SendSd2(read_target_list_16bit, countof(read_target_list_16bit));
}

void iSys400x::Reset()
{
	TaskRadarPoll_Reset();
}

void iSys400x::TaskRadarPoll_Reset()
{
	packetLen = 0;
	ClearRxBuf();
	taskRadarPoll_ = 0;
	ssTimeout.Setms(ISYS_TIMEOUT);
	isConnected = Utils::STATE3::S3_NA;
}

bool iSys400x::TaskRadarPoll()
{
	if (isys400xpwr->iSysPwr == iSys400xPower::PwrSt::PWR_ON)
	{
		TaskRadarPoll_(&taskRadarPoll_);
		if (ssTimeout.IsExpired())
		{
			if (tmrPwrDelay.IsExpired())
			{
				isys400xpwr->AutoPwrOff();
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
		ssTimeout.Setms(ISYS_TIMEOUT);
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
					radarStatus = RadarStatus::INITIALIZING;
					ssTimeout.Setms(ISYS_TIMEOUT);
				}
			}
		} while (radarStatus != RadarStatus::INITIALIZING);
		PT_YIELD();
		do
		{
			do
			{
				CmdStopAcquisition();
				tmrTaskRadar.Setms(100 - 1);
				PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
				taskRadarRS = 0;
				PT_WAIT_TASK(TaskRadarReadSettings(&taskRadarRS));
				if (taskRadarRS != -1)
				{
					PrintDbg(DBG_PRT, "%s settings error, Re-Write settings", uciradar.name.c_str());
					taskRadarWS = 0;
					PT_WAIT_TASK(TaskRadarWriteSettings(&taskRadarWS));
					writeToEEP = true;
					taskRadarWS = 0;
				}
			} while (taskRadarRS != -1);
			PrintDbg(DBG_PRT, "%s settings OK", uciradar.name.c_str());
			if (writeToEEP)
			{
				PrintDbg(DBG_PRT, "%s save settings to eeprom", uciradar.name.c_str());
				CmdSaveToEEprom();
				writeToEEP = false;
			}
			radarStatus = RadarStatus::BUSY;
			ssTimeout.Setms(ISYS_TIMEOUT);
			tmrTaskRadar.Setms(100 - 1);
			PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
			// start
			do
			{
				CmdStartAcquisition();
				tmrTaskRadar.Setms(100 - 1);
				PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
				if (VerifyCmdAck(RADAR_CMD_START))
				{
					radarStatus = RadarStatus::READY;
					ssTimeout.Setms(ISYS_TIMEOUT);
				}
				else
				{
					CmdStopAcquisition();
					tmrTaskRadar.Setms(100 - 1);
					PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
				}
			} while (radarStatus != RadarStatus::READY);
			PT_YIELD();
			// read target list
			tmrTaskRadarSettings.Setms(15 * 60 * 1000);
			do
			{
				ClearRxBuf();
				CmdReadTargetList();
				tmrTaskRadar.Setms(100 - 1);
				PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
				gettimeofday(&pktTime, nullptr);
				if (DecodeTargetFrame() >= 0)
				{
					radarStatus = RadarStatus::EVENT;
					ssTimeout.Setms(ISYS_TIMEOUT);
					if ((GetVdebug() & 1) && vClos.IsValid())
					{
						vClos.Print();
					}
					if ((GetVdebug() & 2) && vAway.IsValid())
					{
						vAway.Print();
					}
				}
				PT_YIELD();
				if (tmrTaskRadarSettings.IsExpired())
				{
					PT_WAIT_TASK(TaskRadarReadSettings(&taskRadarRS));
					if (taskRadarRS != -1)
					{
						taskRadarWS = 1;
						PrintDbg(DBG_LOG, "%s settings error", uciradar.name.c_str());
					}
					tmrTaskRadarSettings.Setms(15 * 60 * 1000);
				}
			} while (taskRadarWS == 0);
		} while (taskRadarWS == 0);
	};
	PT_END();
}

bool iSys400x::TaskRadarReadSettings(int *_ptLine)
{
	PT_BEGIN();
	ClearRxBuf();
	CmdReadFreqChannel();
	tmrTaskRadar.Setms(100 - 1);
	PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
	if (DecodeFreqChannel() != uciradar.radarMode)
	{
		PT_ESCAPE();
	}
	ClearRxBuf();
	CmdReadAppSetting(6);
	tmrTaskRadar.Setms(100 - 1);
	PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
	if (DecodeAppSetting() != 0x0003)
	{
		PT_ESCAPE();
	}
	ClearRxBuf();
	CmdReadAppSetting(7);
	tmrTaskRadar.Setms(100 - 1);
	PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
	if (DecodeAppSetting() != 0)
	{
		PT_ESCAPE();
	}
	PT_END();
}

bool iSys400x::TaskRadarWriteSettings(int *_ptLine)
{
	PT_BEGIN();
	CmdSetFreqChannel();
	tmrTaskRadar.Setms(100 - 1);
	PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
	CmdWriteAppSetting(6, 0x0003); // Clos & Away
	tmrTaskRadar.Setms(100 - 1);
	PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
	CmdWriteAppSetting(7, 0); // OFF
	tmrTaskRadar.Setms(100 - 1);
	PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
	PT_END();
}

int iSys400x::DecodeTargetFrame()
{
	int64_t usec = Timeval2us(pktTime);
	vClos.Reset();
	vClos.usec = usec;
	vAway.Reset();
	vAway.usec = usec;

	if (ReadPacket() <= 0)
	{
		return -1;
	}

	int cnt = 0;
	int fc = (packet[0] == ISYS_FRM_CTRL_SD2 /* check SD2 Frame */) ? 6 /* variable length frames */
																	: 3 /* fixed length frames */;
	int output_number = packet[fc + 1];
	int nrOfTargets = packet[fc + 2];
	/* check for valid amount of targets */
	if (nrOfTargets == 0xff)
	{ // 0xff means clipping, A frame with clipping doesn’t have any targets
		return 0xFF;
	}
	else if (nrOfTargets > MAX_TARGETS || (nrOfTargets * 7 + fc + 3 + 2) != packetLen)
	{
		return -1;
	}
	else if (nrOfTargets == 0)
	{
		return 0;
	}
	auto pData = &packet[fc + 3];
	for (int i = 0; i < nrOfTargets; i++)
	{
		Vehicle v;
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

		if (v.signal >= uciradar.minSignal)
		{
			if (v.speed >= 0)
			{
				if (v.range >= uciradar.minClos && v.range <= uciradar.maxClos && v.speed >= uciradar.minSpeed && v.speed <= uciradar.maxSpeed)
				{
					if (!vClos.IsValid() || v.range < vClos.range)
					{
						memcpy(&vClos, &v, sizeof(Vehicle));
						cnt |= 1;
					}
				}
			}
			else
			{
				if (v.range >= uciradar.minAway && v.range <= uciradar.maxAway && (-v.speed) >= uciradar.minSpeed && (-v.speed) <= uciradar.maxSpeed)
				{
					if (!vAway.IsValid() || v.range < vAway.range)
					{
						memcpy(&vAway, &v, sizeof(Vehicle));
						cnt |= 2;
					}
				}
			}
		}
	}
	return cnt;
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
			list1.vClos[0] = {60, 80, 15000, 12};

			TargetList list2{name, 4001};
			list2.cnt = 2;
			list2.vClos[0] = {60, 80, 14000, 12};
			list2.vClos[1] = {61, 70, 15000, 12};

			TargetList list3{name, 4001};
			list3.cnt = 3;
			list3.vClos[0] = {60, 80, 13000, 12};
			list3.vClos[1] = {61, 70, 14000, 12};
			list3.vClos[2] = {62, 60, 15000, 12};

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

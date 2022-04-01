#include <cstring>
#include <unistd.h>
#include <memory>

#include <radar/iSys.h>

#include <module/MyDbg.h>
#include <module/Epoll.h>
#include <module/Utils.h>
#include <module/ptcpp.h>

using namespace Utils;

using namespace Radar;
using namespace Radar::iSys;

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

int TargetList::DecodeTargetFrame(uint8_t *packet, int packetLen)
{
	cnt = 0;
	gettimeofday(&time, nullptr);
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
	auto pData = &packet[fc + 3];
	for (int i = 0; i < nrOfTargets; i++)
	{
		auto &v = vehicles[cnt];
		// Signal
		v.signal = *pData;
		pData += 1;
		// Velocity
		v.speed = *(int16_t *)pData * 3600 / 100000; // 0.01m/s -> km/h;
		pData += 2;
		// Range: 4004 & 6003 => -32.768 … 32.767 [m]; others => -327.68 … 327.67 [m]
		v.range = *(int16_t *)pData / ((code == 4004 || code == 6003) ? 1000 : 100); // cm/mm => m
		pData += 2;
		// angle
		v.angle = *(int16_t *)pData / 100; // -327.68 … 327.67 [°]
		pData += 2;
		if (v.signal >= MIN_SIGNAL && v.speed >= MIN_SPEED && v.range >= MIN_RANGE)
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
		ptr = Cnvt::PutU16(v.range * 100, ptr);
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

int TargetList::SaveTarget(const char *comment)
{
	if (cnt == 0)
	{ // no vehicle
		switch (flag)
		{
		case -1:
			csv.SaveRadarMeta(time, "Invalid taget list", nullptr);
			break;
		case 0:
			if (hasVehicle)
			{
				hasVehicle = false;
				csv.SaveRadarMeta(time, "There is no vehicle", nullptr);
			}
			break;
		case 0xFF:
			csv.SaveRadarMeta(time, "Clipping", nullptr);
			break;
		}
	}
	else
	{
		char xbuf[1024];
		PrintList(xbuf);
		csv.SaveRadarMeta(time, xbuf, comment);
	}
	return 0;
}

int TargetList::PrintList(char *buf)
{
	int len = sprintf(buf, "%d:", cnt);
	for (int i = 0; i < cnt; i++)
	{
		len += vehicles[i].Print(buf + len);
	}
	return len;
}

iSys400x::iSys400x(UciRadar &uciradar)
	: IRadar(uciradar), targetlist(uciradar.name, uciradar.radarCode)
{
	oprSp = new OprSp(uciradar.radarPort, uciradar.radarBps, nullptr);
	radarStatus = RadarStatus::READY;
}

iSys400x::~iSys400x()
{
	delete oprSp;
}

void iSys400x::SendSd2(const uint8_t *p, int len)
{
	uint8_t buf[6];
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
			if (buf[i] == ISYS_FRM_CTRL_SD2)
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

bool iSys400x::TaskRadarPoll_(int *_ptLine)
{
	PT_BEGIN();
	while (true)
	{
		do
		{
			ClearRxBuf();
			CmdStartAcquisition();
			tmrTaskRadar.Setms(500);
			PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());
		} while (!VerifyCmdAck());
		do
		{
			ClearRxBuf();
			CmdReadTargetList();
			tmrTaskRadar.Setms(100);
			PT_WAIT_UNTIL(tmrTaskRadar.IsExpired());

			if (ReadPacket() > 0)
			{
				ReloadTmrssTimeout();
				int x = targetlist.DecodeTargetFrame(packet, packetLen);
				targetlist.flag = x;
				if (x == 0 || x == 0xFF)
				{
					radarStatus = RadarStatus::EVENT;
					minRangeVehicle = nullptr;
				}
				else if (x > 0)
				{
					minRangeVehicle = &targetlist.vehicles[0];
					for (int i = 1; i < targetlist.cnt; i++)
					{
						auto &v = targetlist.vehicles[i];
						if (v.range < minRangeVehicle->range)
						{
							minRangeVehicle = &targetlist.vehicles[i];
						}
					}
					radarStatus = RadarStatus::EVENT;
				}
			}
		} while (GetStatus() != RadarStatus::NO_CONNECTION);
	};
	PT_END();
}

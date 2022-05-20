#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>
#include <module/SerialPort.h>

using namespace Utils;

#define DISTANCE_NUMBER 8

void UciSettings::LoadConfig()
{
	PrintDbg(DBG_LOG, ">>> Loading 'UciSettings'");
	PATH = DbHelper::Instance().Path();
	PACKAGE = "UciSettings";
	Open();
	DbHelper &db = DbHelper::Instance();
	char option[16];
	int buf[255];
	const char *str;
	int d;
	struct uci_section *uciSec;

	// Controller
	SECTION = _Controller;
	uciSec = GetSection(SECTION);
	freeGiga = GetInt(uciSec, _FreeGiga, 1, 16, true);

	// cloud
	SECTION = _Cloud;
	uciSec = GetSection(SECTION);
	uciCloud.ip = std::string(GetStr(uciSec, _Ip, true));
	uciCloud.port = GetInt(uciSec, _Port, 1024, 65535, true);
	uciCloud.site = std::string(GetStr(uciSec, _Site, true));

	// train
	SECTION = _Train;
	uciSec = GetSection(SECTION);
	uciTrain.monitor = GetInt(uciSec, _Monitor, 1, 2, true);
	{ // range
		str = GetStr(uciSec, _Range);
		int ibuf[2];
		int cnt = Cnvt::GetIntArray(str, 2, ibuf, 1, 15000);
		if (cnt != 2 || ibuf[0] >= ibuf[1])
		{
			throw std::invalid_argument(FmtException("Train.Range error"));
		}
		for (int i = 0; i < cnt; i++)
		{
			uciTrain.range[i] = ibuf[i];
		}
	}
	// monitor%d
	SECTION = sectionBuf;
	for (int i = 0; i < 2; i++)
	{
		sprintf(sectionBuf, "%s%d", _Monitor, i + 1);
		uciSec = GetSection(SECTION);
		auto &c = uciMonitor[i];
		c.name = std::string(sectionBuf);
		str = GetStr(uciSec, _Distance);
		int ibuf[DISTANCE_NUMBER];
		int cnt = Cnvt::GetIntArray(str, DISTANCE_NUMBER, ibuf, -15000, 15000);
		c.distance.resize(cnt);
		for (int i = 0; i < cnt; i++)
		{
			c.distance.at(i) = ibuf[i];
		}
		if (i == 0) // Monitor1
		{
			c.stopTrigger = GetInt(uciSec, _StopTrigger, 300, 5000, true);
			c.stopPass = GetInt(uciSec, _StopPass, c.stopTrigger + 100, 5000, true);
		}
		else
		{
			c.stopPass = GetInt(uciSec, _StopPass, 300, 5000, true);
			c.stopTrigger = GetInt(uciSec, _StopTrigger, c.stopPass + 100, 5000, true);
		}
		c.vstopDelay = GetInt(uciSec, _VstopDelay, 1000, 5000, true);
		c.camRange = GetInt(uciSec, _CamRange, 1, 3, true);
		c.camVstop = GetInt(uciSec, _CamVstop, 1, 3, true);
		c.stkrCapture = GetInt(uciSec, _StkrCapture, 0, 1, true);
		c.stkrDbg1 = GetInt(uciSec, _StkrDbg1, 1, 2, true);
		c.stkrClos = GetInt(uciSec, _StkrClos, 1, 2, true);
		c.stkrAway = GetInt(uciSec, _StkrAway, 0, 2, true);
		c.isysClos = GetInt(uciSec, _iSysClos, 1, 2, true);
		c.isysAway = GetInt(uciSec, _iSYSAway, 0, 2, true);
	}

	for (int i = 0; i < 2; i++)
	{
		// isys
		sprintf(sectionBuf, "%s%d", _iSys, i + 1);
		uciiSys[i].name = std::string(sectionBuf);
		uciSec = GetSection(sectionBuf);
		uciiSys[i].radarPort = GetIndexFromStrz(uciSec, _RadarPort, COM_NAME, COMPORT_SIZE);
		uciiSys[i].radarBps = GetInt(uciSec, _RadarBps, ALLOWEDBPS, STANDARDBPS_SIZE, true);
		uciiSys[i].radarId = GetInt(uciSec, _RadarId, 2, 255, true); // 0 & 1 not allowed
		uciiSys[i].radarMode = GetInt(uciSec, _RadarMode, 0, INT_MAX, true);
		uciiSys[i].radarCode = GetInt(uciSec, _RadarCode, 0, INT_MAX, true);
		uciiSys[i].rangeRise = GetInt(uciSec, _RangeRise, 1, 5000, true);
		uciiSys[i].rangeLast = GetInt(uciSec, _RangeLast, 1, 5000, true);
		uciiSys[i].minClos = GetInt(uciSec, _MinClos, 1, 5000, true);
		uciiSys[i].maxClos = GetInt(uciSec, _MaxClos, uciiSys[i].minAway + 1000, 15000, true);
		uciiSys[i].minAway = GetInt(uciSec, _MinAway, 1, 5000, true);
		uciiSys[i].maxAway = GetInt(uciSec, _MaxAway, uciiSys[i].minAway + 1000, 15000, true);
		uciiSys[i].minSignal = GetInt(uciSec, _MinSignal, 1, 99, true);
		uciiSys[i].minSpeed = GetInt(uciSec, _MinSpeed, 1, 99, true);
		uciiSys[i].maxSpeed = GetInt(uciSec, _MaxSpeed, uciiSys[i].minSpeed + 1, 255, true);
		uciiSys[i].cmErr = GetInt(uciSec, _CmErr, 1, 5000, true);
	}

	// stalker
	for (int i = 0; i < 2; i++)
	{
		sprintf(sectionBuf, "%s%d", _Stalker, i + 1);
		uciStalker[i].name = std::string(sectionBuf);
		uciSec = GetSection(sectionBuf);
		uciStalker[i].name = std::string(sectionBuf);
		uciStalker[i].radarPort = GetIndexFromStrz(uciSec, _RadarPort, COM_NAME, COMPORT_SIZE);
		uciStalker[i].radarBps = GetInt(uciSec, _RadarBps, ALLOWEDBPS, STANDARDBPS_SIZE, true);
	}

	int rcom[4] = {
		uciiSys[0].radarPort,
		uciStalker[0].radarPort,
		uciiSys[1].radarPort,
		uciStalker[1].radarPort};

	for (int i = 0; i < 3; i++)
	{
		for (int j = i + 1; j < 4; j++)
		{
			if (rcom[i] == rcom[j])
			{
				throw std::invalid_argument(FmtException("RadarPort error : 2 radars on same port : %s", COM_NAME[rcom[i]]));
			}
		}
	}

	Close();
	Dump();
}

void UciSettings::Dump()
{
	PrintDash('<');
	printf("%s/%s\n", PATH, PACKAGE);
	char buf[1024];

	printf("%s:\n", _Controller);
	PrintOption_d(_FreeGiga, freeGiga);

	printf("%s:\n", _Cloud);
	PrintOption_str(_Ip, uciCloud.ip.c_str());
	PrintOption_d(_Port, uciCloud.port);
	PrintOption_str(_Site, uciCloud.site.c_str());

	printf("%s:\n", _Train);
	PrintOption_d(_Monitor, uciTrain.monitor);
	printf("\t%s \t'%d, %d'\n", _Range, uciTrain.range[0], uciTrain.range[1]);

	for (int i = 0; i < 2; i++)
	{
		auto &c = uciMonitor[i];
		printf("%s:\n", c.name.c_str());
		int len = 0;
		for (int i = 0; i < c.distance.size(); i++)
		{
			len += sprintf(buf + len, (i == 0) ? "%d" : ",%d", c.distance[i]);
		}
		printf("\t%s \t'%s'\n", _Distance, buf);
		PrintOption_d(_StopTrigger, c.stopTrigger);
		PrintOption_d(_StopPass, c.stopPass);
		PrintOption_d(_CamRange, c.camRange);
		PrintOption_d(_CamVstop, c.camVstop);
		PrintOption_d(_VstopDelay, c.vstopDelay);
		PrintOption_d(_VstopSpeed, c.vstopSpeed);
		PrintOption_d(_StkrCapture, c.stkrCapture);
		PrintOption_d(_StkrDbg1, c.stkrDbg1);
		PrintOption_d(_StkrClos, c.stkrClos);
		PrintOption_d(_StkrAway, c.stkrAway);
		PrintOption_d(_iSysClos, c.isysClos);
		PrintOption_d(_iSYSAway, c.isysAway);
	}
	for (int i = 0; i < 2; i++)
		PrintRadar(uciiSys[i]);
	for (int i = 0; i < 2; i++)
		PrintRadar(uciStalker[i]);

	PrintDash('>');
}

void UciSettings::PrintRadar(UciRadar &radar)
{
	printf("%s:\n", radar.name.c_str());
	PrintOption_str(_RadarPort, COM_NAME[radar.radarPort]);
	PrintOption_d(_RadarBps, radar.radarBps);
	if (radar.radarId >= 0)
	{
		PrintOption_d(_RadarId, radar.radarId);
	}
	if (radar.radarMode >= 0)
	{
		PrintOption_d(_RadarMode, radar.radarMode);
	}
	if (radar.radarCode >= 0)
	{
		PrintOption_d(_RadarCode, radar.radarCode);
	}
	if (radar.rangeLast >= 0)
	{
		PrintOption_d(_RangeLast, radar.rangeLast);
	}
	if (radar.rangeRise >= 0)
	{
		PrintOption_d(_RangeRise, radar.rangeRise);
	}
	if (radar.minClos >= 0)
	{
		PrintOption_d(_MinClos, radar.minClos);
	}
	if (radar.maxClos >= 0)
	{
		PrintOption_d(_MaxClos, radar.maxClos);
	}
	if (radar.minAway >= 0)
	{
		PrintOption_d(_MinAway, radar.minAway);
	}
	if (radar.maxAway >= 0)
	{
		PrintOption_d(_MaxAway, radar.maxAway);
	}
	if (radar.minSpeed >= 0)
	{
		PrintOption_d(_MinSpeed, radar.minSpeed);
	}
	if (radar.maxSpeed >= 0)
	{
		PrintOption_d(_MaxSpeed, radar.maxSpeed);
	}
	if (radar.minSignal >= 0)
	{
		PrintOption_d(_MinSignal, radar.minSignal);
	}
	if (radar.cmErr >= 0)
	{
		PrintOption_d(_CmErr, radar.cmErr);
	}
}

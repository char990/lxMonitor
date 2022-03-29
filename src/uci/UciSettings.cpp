#include <cstdio>
#include <cstring>
#include <uci.h>
#include <uci/DbHelper.h>
#include <module/Utils.h>
#include <module/MyDbg.h>
#include <module/SerialPort.h>

using namespace Utils;

#define DISTANCE_NUMBER 8

UciSettings::UciSettings()
{
}

UciSettings::~UciSettings()
{
}

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

	// cloud
	SECTION = _Cloud;
	uciSec = GetSection(SECTION);
	uciCloud.ip = std::string(GetStr(uciSec, _Ip, true));
	uciCloud.port = GetInt(uciSec, _Port, 1024, 65535, true);
	uciCloud.site = std::string(GetStr(uciSec, _Site, true));

	// monitor%d
	SECTION = sectionBuf;
	for (int i = 0; i < 2; i++)
	{
		sprintf(sectionBuf, "%s%d", _Monitor, i + 1);
		uciSec = GetSection(SECTION);
		auto &c = uciMonitor[i];
		c.name = std::string(sectionBuf);
		c.output = GetInt(uciSec, _Output, 1, 2, true);
		str = GetStr(uciSec, _Distance);
		int ibuf[DISTANCE_NUMBER];
		int cnt = Cnvt::GetIntArray(str, DISTANCE_NUMBER, ibuf, 1, 999);
		c.distance.resize(cnt);
		for (int i = 0; i < cnt; i++)
		{
			c.distance.at(i) = ibuf[i];
		}
		const char * isys = GetStr(uciSec, _iSys);
		const char * stalker = GetStr(uciSec, _Stalker);
		// isys
		uciSec = GetSection(isys);
		c.iSys.name = std::string(isys);
		c.iSys.radarPort = GetIndexFromStrz(uciSec, _RadarPort, COM_NAME, COMPORT_SIZE);
		c.iSys.radarBps = GetInt(uciSec, _RadarBps, ALLOWEDBPS, STANDARDBPS_SIZE, true);
		c.iSys.radarId = GetInt(uciSec, _RadarId, 2, 255, true); // 0 & 1 not allowed
		c.iSys.radarMode = GetInt(uciSec, _RadarMode, INT_MIN, INT_MAX, true);
		c.iSys.radarCode = GetInt(uciSec, _RadarCode, INT_MIN, INT_MAX, true);

		// stalker
		uciSec = GetSection(stalker);
		c.stalker.name = std::string(stalker);
		c.stalker.radarPort = GetIndexFromStrz(uciSec, _RadarPort, COM_NAME, COMPORT_SIZE);
		c.stalker.radarBps = GetInt(uciSec, _RadarBps, ALLOWEDBPS, STANDARDBPS_SIZE, true);
		c.stalker.radarId = GetInt(uciSec, _RadarId, 0, 255, true);
		c.stalker.radarMode = GetInt(uciSec, _RadarMode, INT_MIN, INT_MAX, true);
		c.stalker.radarCode = GetInt(uciSec, _RadarCode, INT_MIN, INT_MAX, true);
	}

	int rcom[4] = {
		uciMonitor[0].iSys.radarPort,
		uciMonitor[0].stalker.radarPort,
		uciMonitor[1].iSys.radarPort,
		uciMonitor[1].stalker.radarPort};

	for (int i = 0; i < 3; i++)
	{
		for (int j = i + 1; j < 4; j++)
		{
			if (rcom[i]==rcom[j])
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

	printf("%s:\n", _Cloud);
	PrintOption_str(_Ip, uciCloud.ip.c_str());
	PrintOption_d(_Port, uciCloud.port);
	PrintOption_str(_Site, uciCloud.site.c_str());
	
	for (int i = 0; i < 2; i++)
	{
		auto &c = uciMonitor[i];
		printf("%s:\n", c.name.c_str());
		PrintOption_str(_iSys, c.iSys.name.c_str());
		PrintOption_str(_Stalker, c.stalker.name.c_str());
		PrintOption_d(_Output, c.output);
		int len = 0;
		for (int i = 0; i < c.distance.size(); i++)
		{
			len += sprintf(buf + len, (i == 0) ? "%d" : ",%d", c.distance[i]);
		}
		printf("\t%s \t'%s'\n", _Distance, buf);

		PrintRadar(c.iSys);
		PrintRadar(c.stalker);
	}
	PrintDash('>');
}

void UciSettings::PrintRadar(UciRadar &radar)
{
	printf("%s:\n", radar.name.c_str());
	PrintOption_str(_RadarPort, COM_NAME[radar.radarPort]);
	PrintOption_d(_RadarBps, radar.radarBps);
	PrintOption_d(_RadarId, radar.radarId);
	PrintOption_d(_RadarMode, radar.radarMode);
	PrintOption_d(_RadarCode, radar.radarCode);
}

#pragma once

#include <string>
#include <vector>

#include <uci/UciCfg.h>
#include <module/Utils.h>


class UciCloud
{
public:
    std::string ip;
    int port;
    std::string site;
};

class UciRadar
{
public:
    std::string name;
    int radarPort;
    int radarBps;
    uint8_t radarId;
    int radarMode;
    int radarCode;
};

class UciMonitor
{
public:
    std::string name;
    UciRadar iSys;
    UciRadar stalker;
    int output;
    std::vector<int> distance;
};

class UciSettings : public UciCfg
{
public:
    UciSettings();
    ~UciSettings();

    void LoadConfig() override;

    void Dump() override;

    UciCloud uciCloud;
    UciMonitor uciMonitor[2];

private:
    char sectionBuf[16];

    const char *_Cloud = "Cloud";           // section
    const char *_Ip = "Ip";
    const char *_Port = "Port";
    const char *_Site = "Site";

    const char *_Monitor = "Monitor";       // section
    const char *_iSys = "iSys";
    const char *_Stalker = "Stalker";
    const char *_Output = "Output";
    const char *_Distance = "Distance";

    // Radar    
    const char *_RadarPort = "RadarPort";
    const char *_RadarBps = "RadarBps";
    const char *_RadarId = "RadarId";
    const char *_RadarMode = "RadarMode";
    const char *_RadarCode = "RadarCode";

    void PrintRadar(UciRadar & radar);
};

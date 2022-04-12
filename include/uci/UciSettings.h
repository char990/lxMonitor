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
    int radarId{-1};    // -1 means may not be used
    int radarMode{-1};
    int radarCode{-1};
    int rangeLast{-1};
    int rangeRise{-1};
    int minRange{-1};
    int minSpeed{-1};
    int minSignal{-1};
    int cmErr{-1};
};

class UciMonitor
{
public:
    std::string name;
    UciRadar iSys;
    UciRadar stalker;
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

    unsigned int FreeGiga() { return freeGiga;};

private:

    char sectionBuf[16];

    const char *_Controller = "Controller";           // section
    const char *_FreeGiga = "FreeGiga";
    unsigned int freeGiga;
    
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
    const char *_RangeRise = "RangeRise";
    const char *_RangeLast = "RangeLast";
    const char *_MinRange = "MinRange";
    const char *_MinSpeed = "MinSpeed";
    const char *_MinSignal = "MinSignal";
    const char *_CmErr = "CmErr";


    void PrintRadar(UciRadar & radar);
};

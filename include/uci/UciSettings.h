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
    int minClos{-1};
    int maxClos{-1};
    int minAway{-1};
    int maxAway{-1};
    int minSpeed{-1};
    int maxSpeed{-1};
    int minSignal{-1};
    int cmErr{-1};
};

class UciMonitor
{
public:
    std::string name;
    std::vector<int> distance;
    int stopTrigger;
    int stopPass;
    int vstopDelay;
    int vstopSpeed;
    int camRange;
    int camVstop;
    int stkrCapture;
    int stkrDbg1;
    int stkrClos;
    int stkrAway;
    int isysClos;
    int isysAway;
};

class UciTrian
{
public:
    int monitor;
    int range[2];
};

class UciSettings : public UciCfg
{
public:
    UciSettings(){};
    ~UciSettings(){};

    void LoadConfig() override;

    void Dump() override;

    UciCloud uciCloud;
    UciMonitor uciMonitor[2];
    UciTrian uciTrain;
    UciRadar uciiSys[2];
    UciRadar uciStalker[2];
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

    const char *_Train = "Train";           // section
    //const char *_Monitor = "Monitor";
    const char *_Range = "Range";

    const char *_Monitor = "Monitor";       // section
    const char *_Distance = "Distance";
    const char *_StopTrigger = "StopTrigger";
    const char *_StopPass = "StopPass";
    const char *_VstopDelay = "VstopDelay";
    const char *_VstopSpeed = "VstopSpeed";
    const char *_CamRange = "CamRange";
    const char *_CamVstop = "CamVstop";
    const char *_StkrCapture = "StkrCapture";
    const char *_StkrDbg1 = "StkrDbg1";
    const char *_StkrClos = "StkrClos";
    const char *_StkrAway = "StkrAway";
    const char *_iSysClos = "iSysClos";
    const char *_iSYSAway = "iSysAway";
    // Radar    
    const char *_iSys = "iSys";       // section
    const char *_Stalker = "Stalker";       // section
    const char *_RadarPort = "RadarPort";
    const char *_RadarBps = "RadarBps";
    const char *_RadarId = "RadarId";
    const char *_RadarMode = "RadarMode";
    const char *_RadarCode = "RadarCode";
    const char *_RangeRise = "RangeRise";
    const char *_RangeLast = "RangeLast";
    const char *_MinClos = "MinClos";
    const char *_MaxClos = "MaxClos";
    const char *_MinAway = "MinAway";
    const char *_MaxAway = "MaxAway";
    const char *_MinSpeed = "MinSpeed";
    const char *_MaxSpeed = "MaxSpeed";
    const char *_MinSignal = "MinSignal";
    const char *_CmErr = "CmErr";


    void PrintRadar(UciRadar & radar);
};

#pragma once

#include <string>


extern const char * PHOTO_TAKEN;
extern const char * NO_VEHICLE;

class SaveCSV
{
public:
    SaveCSV(std::string filename);
    ~SaveCSV();

    int SaveRadarMeta(struct timeval &time, const char *comment, const char *meta);
    int SaveRadarMeta(int64_t &us, const char *comment, const char *meta);
    int SaveRadarMeta(const char *comment, const char *meta);

private:
    std::string filename;
    int csvfd{-1};
    char lastdate[11]{0}; // dd/mm/yyyy
};

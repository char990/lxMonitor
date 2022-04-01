#pragma once

#include <string>


extern const char * PHOTO_TAKEN;

class SaveCSV
{
public:
    SaveCSV(std::string filename);
    ~SaveCSV();

    int SaveRadarMeta(struct timeval &time, const char *meta, const char *comment);

private:
    std::string filename;
    int csvfd{-1};
    char lastdate[11]{0}; // dd/mm/yyyy
};

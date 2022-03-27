#pragma once


#include <uci/UciSettings.h>


class DbHelper
{
public:
    DbHelper(DbHelper const &) = delete;
    void operator=(DbHelper const &) = delete;
    static DbHelper &Instance()
    {
        static DbHelper instance;
        return instance;
    }

    void Init(const char * dbPath);
    
    UciSettings & GetUciSettings() { return uciSettings; };

    const char * Path(){ return dbPath; };

protected:

    UciSettings uciSettings;

private:
    DbHelper(){};
    ~DbHelper(){}

    const char * dbPath;
};


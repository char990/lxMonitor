#pragma once


#include <cstdio>
#include <string>
#include <stdexcept>

// PrintDbg level
enum DBG_LEVEL { DBG_HB, DBG_PRT, DBG_LOG};

extern std::string FmtException(const char * fmt, ...);
extern int PrintDbg(DBG_LEVEL level, const char * fmt, ...);
extern void PrintDash(char c);

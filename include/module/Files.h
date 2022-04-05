#pragma once

#include <vector>
#include <string>

class Files
{
public:
    static int GetDirs(std::string path, std::vector<std::string> &subdir, int depth = 1);
};

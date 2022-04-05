#include <module/Files.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

#include <cstring>

#include <module/MyDbg.h>

int Files::GetDirs(std::string path, std::vector<std::string> &subdir, int depth)
{
    DIR *dir = opendir(path.c_str());
    if (dir == nullptr)
    {
        PrintDbg(DBG_LOG, "Can't open '%s'", path.c_str());
        return -1;
    }
    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr)
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0 // skip "." and ".."
            && entry->d_type == DT_DIR)                                         // this entry is a directory
        {
            if (path.back() == '/')
            {
                subdir.push_back(path + entry->d_name);
            }
            else
            {
                subdir.push_back(path + "/" + entry->d_name);
            }
        }
    }
    closedir(dir);
    return 0;
}

#if 0
#include <3rdparty/catch2/EnableTest.h>
#if _ENABLE_TEST_ == 1
#include <3rdparty/catch2/catch.hpp>
#include <Consts.h>
#include <module/Utils.h>

using namespace Utils;

#define TEST_FILES

#ifdef TEST_FILES
TEST_CASE("Class Files", "[Files]")
{
    std::string path = std::string(metapath);
    SECTION("GetDirs")
    {
        std::vector<std::string> subdir;
        int x = Files::GetDirs(std::string(metapath), subdir);
        printf("subdir.size()=%d\n", subdir.size());
        StrFn::vsPrint(&subdir);
        printf("sort subdir\n");
        std::sort(subdir.begin(), subdir.end());
        StrFn::vsPrint(&subdir);
        REQUIRE(x == 0);
    }
}
#endif

#endif

#endif

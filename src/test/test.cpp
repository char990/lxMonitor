#include <3rdparty/catch2/enable_test.h>
#ifdef CATCH2TEST
#define CATCH_CONFIG_MAIN
#include <time.h>
#include <3rdparty/catch2/catch.hpp>
#include <module/Utils.h>

using namespace Utils;

TEST_CASE("Class Utils::Bits", "[Bits]")
{
    Bits b1(256);

    SECTION("256 bits empty")
    {
        REQUIRE(b1.Size() == 256);
        REQUIRE(b1.Data() != nullptr);
        for (int i = 0; i < 256; i++)
        {
            REQUIRE(b1.GetBit(i) == false);
        }
        REQUIRE(b1.GetMaxBit() == -1);
    }

    SECTION("SetBit:0")
    {
        b1.SetBit(0);
        REQUIRE(b1.GetMaxBit() == 0);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        REQUIRE(b1.GetMaxBit() == 7);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 7:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        REQUIRE(b1.GetMaxBit() == 255);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 7:
            case 255:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:1")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(1);
        REQUIRE(b1.GetMaxBit() == 255);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 7:
            case 255:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:7")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(7);
        REQUIRE(b1.GetMaxBit() == 255);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
            case 255:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:7,255")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(7);
        b1.ClrBit(255);
        REQUIRE(b1.GetMaxBit() == 0);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 0:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("SetBit:0,7,255 and ClrBit:7,255,0")
    {
        b1.SetBit(0);
        b1.SetBit(7);
        b1.SetBit(255);
        b1.ClrBit(7);
        b1.ClrBit(255);
        b1.ClrBit(0);
        REQUIRE(b1.GetMaxBit() == -1);
        for (int i = 0; i < 256; i++)
        {
            REQUIRE(b1.GetBit(i) == false);
        }
    }

    SECTION("SetBit=8,16,63")
    {
        b1.SetBit(8);
        b1.SetBit(16);
        b1.SetBit(63);
        REQUIRE(b1.GetMaxBit() == 63);
        for (int i = 0; i < 256; i++)
        {
            switch (i)
            {
            case 8:
            case 16:
            case 63:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }

    SECTION("b1.ClrAll")
    {
        b1.ClrAll();
        REQUIRE(b1.GetMaxBit() == -1);
        for (int i = 0; i < 256; i++)
        {
            REQUIRE(b1.GetBit(i) == false);
        }
    }

    SECTION("b2=72-bit.SetBit(0,1,7,8,31,32,63)")
    {
        Bits b2;
        b2.Init(72);
        b2.SetBit(0);
        b2.SetBit(1);
        b2.SetBit(7);
        b2.SetBit(8);
        b2.SetBit(31);
        b2.SetBit(32);
        b2.SetBit(63);
        REQUIRE(b2.GetMaxBit() == 63);
        for (int i = 0; i < b2.Size(); i++)
        {
            switch (i)
            {
            case 0:
            case 1:
            case 7:
            case 8:
            case 31:
            case 32:
            case 63:
                REQUIRE(b2.GetBit(i) == true);
                break;
            default:
                REQUIRE(b2.GetBit(i) == false);
                break;
            }
        }
        b1.Clone(b2);
        REQUIRE(b1.GetMaxBit() == 63);
        REQUIRE(b1.Size() == b2.Size());
        for (int i = 0; i < b1.Size(); i++)
        {
            switch (i)
            {
            case 0:
            case 1:
            case 7:
            case 8:
            case 31:
            case 32:
            case 63:
                REQUIRE(b1.GetBit(i) == true);
                break;
            default:
                REQUIRE(b1.GetBit(i) == false);
                break;
            }
        }
    }
}


#endif

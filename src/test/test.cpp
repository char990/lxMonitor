#include <3rdparty/catch2/enable_test.h>
#if CATCH2TEST != 0
#define CATCH_CONFIG_MAIN
#include <time.h>
#include <string>
#include <3rdparty/catch2/catch.hpp>
#include <module/Utils.h>
#include <radar/Stalker.h>
#include <radar/iSys.h>
namespace Radar::Stalker
{
    TEST_CASE("Class DBG1", "[DBG1]")
    {
        DBG1 dbg1;

        SECTION("legal DBG1")
        {
            dbg1.Init((const uint8_t *)"T00 1027 C  5 C  5 C  5 70    1 ");
            REQUIRE(dbg1.id == 1027);
            dbg1.Init((const uint8_t *)"T01 1999 C  5 C  5 C  5 70   89 ");
            REQUIRE(dbg1.id == 1999);
            dbg1.Init((const uint8_t *)"T02 8888 C  5 C  5 C  5 70    1 ");
            REQUIRE(dbg1.id == 8888);
        }
        SECTION("illegal DBG1")
        {
            dbg1.Init((const uint8_t *)"T00 1027 A  5 C  5 C  5 70    1 ");
            REQUIRE(dbg1.id == -1);
            dbg1.Init((const uint8_t *)"T00 61027 C  5 C  5 C  5 70    1 ");
            REQUIRE(dbg1.id == -1);
            dbg1.Init((const uint8_t *)"T00 1027 A  5 C  5 C  5 70 10001 ");
            REQUIRE(dbg1.id == -1);
        }
    }

    TEST_CASE("Class VehicleList", "[VehicleList]")
    {
        auto name = std::string("TESTSTALKER1");
        VehicleList vl(name);

        SECTION("legal DBG1")
        {
            vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    1 ");
            REQUIRE(vl.vlist.size() == 1);
            vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   89 ");
            REQUIRE(vl.vlist.size() == 2);
            vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    1 ");
            REQUIRE(vl.vlist.size() == 3);
            vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    2 ");
            REQUIRE(vl.vlist.size() == 3);
            vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   90 ");
            REQUIRE(vl.vlist.size() == 3);
            vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    2 ");
            REQUIRE(vl.vlist.size() == 3);
            printf("No vehicle\n");
            vl.PushDgb1("\0");
            REQUIRE(vl.vlist.size() == 0);
            vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    1 ");
            REQUIRE(vl.vlist.size() == 1);
            vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   89 ");
            REQUIRE(vl.vlist.size() == 2);
            vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    1 ");
            REQUIRE(vl.vlist.size() == 3);
            vl.PushDgb1("T00 1027 C  5 C  5 C  5 70    2 ");
            REQUIRE(vl.vlist.size() == 3);
            vl.PushDgb1("T01 1999 C  5 C  5 C  5 70   90 ");
            REQUIRE(vl.vlist.size() == 3);
            vl.PushDgb1("T02 8888 C  5 C  5 C  5 70    2 ");
            REQUIRE(vl.vlist.size() == 3);
            printf("sleep(3)\n");
            sleep(3);
            printf("New vehicle\n");
            vl.PushDgb1("T00 9027 C  5 C  5 C  5 70    2 ");
            REQUIRE(vl.vlist.size() == 1);
        }
    }
}

namespace Radar::iSys
{
    TEST_CASE("Class iSys400x", "[iSys400x]")
    {
        uint8_t packet[MAX_PACKET_SIZE];
        int packetLen;

        TargetList list1{4002};
        list1.cnt = 1;
        list1.vehicles[0] = {60, 80, 150, 12};

        TargetList list2{4002};
        list2.cnt = 2;
        list2.vehicles[0] = {60, 80, 140, 12};
        list2.vehicles[1] = {61, 70, 150, 12};

        TargetList list3{4002};
        list3.cnt = 3;
        list3.vehicles[0] = {60, 80, 130, 12};
        list3.vehicles[1] = {61, 70, 140, 12};
        list3.vehicles[2] = {62, 60, 150, 12};

        TargetList target{4002};

        SECTION("DecodeTargetFrame")
        {
            int x;
            list1.MakeTargetMsg(packet, &packetLen);
            x = target.DecodeTargetFrame(packet, packetLen);
            REQUIRE(x == 1);
            list2.MakeTargetMsg(packet, &packetLen);
            x = target.DecodeTargetFrame(packet, packetLen);
            REQUIRE(x == 2);
            list3.MakeTargetMsg(packet, &packetLen);
            x = target.DecodeTargetFrame(packet, packetLen);
            REQUIRE(x == 3);
        }
    }
}

#if 0
namespace Utils
{

    TEST_CASE("Class Utils::Bits", "[Bits]")
    {
        Bits b1(256);

        SECTION("256 bits empty")
        {
            REQUIRE(b1.Size() == 256);
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
}

#endif

#endif

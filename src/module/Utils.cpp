#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <cerrno>
#include <ctime>
#include <string>
#include <sstream>
#include <cstdarg>
#include <module/Utils.h>
#include <module/MyDbg.h>

using namespace Utils;
using namespace std;

const uint32_t Utils::MASK_BIT[32] = {
    0x00000001, 0x00000002, 0x00000004, 0x00000008, 0x00000010, 0x00000020, 0x00000040, 0x00000080,
    0x00000100, 0x00000200, 0x00000400, 0x00000800, 0x00001000, 0x00002000, 0x00004000, 0x00008000,
    0x00010000, 0x00020000, 0x00040000, 0x00080000, 0x00100000, 0x00200000, 0x00400000, 0x00800000,
    0x01000000, 0x02000000, 0x04000000, 0x08000000, 0x10000000, 0x20000000, 0x40000000, 0x80000000};

int Check::HexStr(uint8_t *frm, int len)
{
    if (len < 2 || len & 1)
    {
        return -1;
    }
    for (int i = 0; i < len; i++)
    {
        if (!isxdigit(*frm++))
        {
            return -2;
        }
    }
    return 0;
}

int Check::Text(uint8_t *frm, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (*frm < 0x20 || *frm > 0x7F)
        {
            return -1;
        }
        *frm++;
    }
    return 0;
}

uint8_t Cnvt::Reverse(uint8_t n)
{
    static uint8_t lookup[16] = {
        0x0, 0x8, 0x4, 0xc, 0x2, 0xa, 0x6, 0xe,
        0x1, 0x9, 0x5, 0xd, 0x3, 0xb, 0x7, 0xf};
    // Reverse the top and bottom nibble then swap them.
    return (lookup[n & 0b1111] << 4) | lookup[n >> 4];
}

int Cnvt::ParseToU8(const char *p)
{
    int k = 0;
    for (int i = 0; i < 2; i++)
    {
        int j;
        if (*p >= '0' && *p <= '9') // '0' - '9'
        {
            j = (*p - '0');
        }
        else if (*p >= 'A' && *p <= 'F') // 'A' - 'F'
        {
            j = (*p - 'A' + 0x0A);
        }
        else if (*p >= 'a' && *p <= 'f') // 'a' - 'f'
        {
            j = (*p - 'a' + 0x0A);
        }
        else
        {
            return -1;
        }
        k = k * 16 + j;
        p++;
    }
    return k;
}

int Cnvt::ParseToU8(const char *src, uint8_t *dst, int srclen)
{
    if ((srclen & 1) == 1 || srclen <= 0)
        return -1;
    int len = srclen / 2;
    for (int i = 0; i < len; i++)
    {
        int x = ParseToU8(src);
        if (x < 0)
            return -1;
        *dst = x;
        dst++;
        src += 2;
    }
    return 0;
}

int Cnvt::ParseToU16(const char *src)
{
    int k = 0;
    for (int i = 0; i < 2; i++)
    {
        int x = ParseToU8(src);
        if (x < 0)
            return -1;
        k = k * 0x100 + x;
        src += 2;
    }
    return k;
}

int64_t Cnvt::ParseToU32(const char *src)
{
    int64_t k = 0;
    for (int i = 0; i < 4; i++)
    {
        int x = ParseToU8(src);
        if (x < 0)
            return -1;
        k = k * 0x100 + x;
        src += 2;
    }
    return k;
}

char *Cnvt::ParseToAsc(uint8_t h, char *p)
{
    static char ASC[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};
    *p++ = ASC[(h >> 4)];
    *p++ = ASC[h & 0x0F];
    return p;
}

char *Cnvt::ParseToAsc(uint8_t *src, char *dst, int srclen)
{
    for (int i = 0; i < srclen; i++)
    {
        ParseToAsc(*src, dst);
        src++;
        dst += 2;
    }
    return dst;
}

char *Cnvt::ParseToStr(uint8_t *src, char *dst, int srclen)
{
    char *p = ParseToAsc(src, dst, srclen);
    *p = '\0';
    return p;
}

char *Cnvt::ParseU16ToAsc(uint16_t h, char *p)
{
    p = ParseToAsc(h / 0x100, p);
    p = ParseToAsc(h & 0xFF, p);
    return p;
}

char *Cnvt::ParseU32ToAsc(uint32_t h, char *p)
{
    p = ParseToAsc(h / 0x1000000, p);
    p = ParseToAsc(h / 0x10000, p);
    p = ParseToAsc(h / 0x100, p);
    p = ParseToAsc(h & 0xFF, p);
    return p;
}

int Cnvt::GetIntArray(const char *src, int srcmax, int *dst, int min, int max)
{
    char buf[4096];
    memcpy(buf, src, 4095);
    buf[4096] = '\0'; // copy max 4095 chars
    char delim[] = ",:;. ";
    char *ptr = strtok(buf, delim);
    int cnt = 0;
    while (ptr != NULL && cnt < srcmax)
    {
        errno = 0;
        int x = strtol(ptr, nullptr, 0);
        if (errno == 0 && x >= min && x <= max)
        {
            *dst++ = x;
            cnt++;
        }
        else
        {
            break;
        }
        ptr = strtok(NULL, delim);
    }
    return cnt;
}

uint16_t Cnvt::GetU16(uint8_t *p)
{
    return (*p) * 0x100 + (*(p + 1));
}

uint8_t *Cnvt::PutU16(uint16_t v, uint8_t *p)
{
    *p++ = v / 0x100;
    *p++ = v & 0xFF;
    return p;
}

uint32_t Cnvt::GetU32(uint8_t *p)
{
    uint32_t x = 0;
    for (int i = 0; i < 4; i++)
    {
        x *= 0x100;
        x += *p++;
    }
    return x;
}

uint8_t *Cnvt::PutU32(uint32_t v, uint8_t *p)
{
    p += 3;
    for (int i = 0; i < 4; i++)
    {
        *p-- = v & 0xFF;
        v >>= 8;
    }
    return p + 5;
}

void Cnvt::split(const string &s, vector<string> &tokens, const string &delimiters)
{
    string::size_type lastPos = s.find_first_not_of(delimiters, 0);
    string::size_type pos = s.find_first_of(delimiters, lastPos);
    while (string::npos != pos || string::npos != lastPos)
    {
        tokens.push_back(s.substr(lastPos, pos - lastPos));
        lastPos = s.find_first_not_of(delimiters, pos);
        pos = s.find_first_of(delimiters, lastPos);
    }
}

uint16_t Cnvt::SwapU16(uint16_t v)
{
    return ((v & 0xFF) * 0x100 + v / 0x100);
}

const uint8_t Crc::crc8_table[256] =
    {
        0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
        0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
        0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
        0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
        0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
        0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
        0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
        0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
        0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
        0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
        0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
        0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
        0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
        0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
        0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
        0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
        0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
        0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
        0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
        0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
        0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
        0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
        0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
        0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
        0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
        0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
        0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
        0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
        0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
        0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
        0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
        0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3};

uint8_t Crc::Crc8(uint8_t *buf, int len, uint8_t precrc)
{
    uint8_t crc = precrc;
    for (int index = 0; index < len; index++)
    {
        crc = crc8_table[crc ^ buf[index]];
    }
    return crc;
}

uint16_t Crc::Crc16(const uint16_t *table, uint8_t *buf, int len,
                    uint16_t init, bool refIn, bool refOut, uint16_t xorOut)
{
    uint16_t crc = init;
    if (refIn)
    {
        crc = Cnvt::Reverse(crc >> 8) + (Cnvt::Reverse(crc) * 0x100);
        for (int index = 0; index < len; index++)
        {
            crc = (uint16_t)((crc << 8) ^ table[((crc >> 8) ^ Cnvt::Reverse(buf[index])) & 255]);
        }
    }
    else
    {
        for (int index = 0; index < len; index++)
        {
            crc = (uint16_t)((crc << 8) ^ table[((crc >> 8) ^ buf[index]) & 255]);
        }
    }
    if (refOut)
    {
        crc = Cnvt::Reverse(crc >> 8) + (Cnvt::Reverse(crc) * 0x100);
    }
    return (crc ^ xorOut);
}

const uint16_t Crc::crc16_1021[256] =
    {
        0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5, 0x60c6, 0x70e7,
        0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad, 0xe1ce, 0xf1ef,
        0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294, 0x72f7, 0x62d6,
        0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c, 0xf3ff, 0xe3de,
        0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7, 0x44a4, 0x5485,
        0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf, 0xc5ac, 0xd58d,
        0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6, 0x5695, 0x46b4,
        0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe, 0xd79d, 0xc7bc,
        0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861, 0x2802, 0x3823,
        0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969, 0xa90a, 0xb92b,
        0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50, 0x3a33, 0x2a12,
        0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58, 0xbb3b, 0xab1a,
        0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03, 0x0c60, 0x1c41,
        0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b, 0x8d68, 0x9d49,
        0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32, 0x1e51, 0x0e70,
        0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a, 0x9f59, 0x8f78,
        0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d, 0xf14e, 0xe16f,
        0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025, 0x7046, 0x6067,
        0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c, 0xe37f, 0xf35e,
        0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214, 0x6277, 0x7256,
        0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f, 0xd52c, 0xc50d,
        0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
        0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e, 0xc71d, 0xd73c,
        0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676, 0x4615, 0x5634,
        0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9, 0xb98a, 0xa9ab,
        0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1, 0x3882, 0x28a3,
        0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8, 0xabbb, 0xbb9a,
        0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0, 0x2ab3, 0x3a92,
        0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b, 0x9de8, 0x8dc9,
        0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83, 0x1ce0, 0x0cc1,
        0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba, 0x8fd9, 0x9ff8,
        0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2, 0x0ed1, 0x1ef0};

uint16_t Crc::Crc16_1021(uint8_t *buf, int len, uint16_t precrc)
{
    return Crc16(crc16_1021, buf, len, precrc, false, false, 0);
}

const uint16_t Crc::crc16_8005[256] =
    {
        0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011,
        0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
        0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072,
        0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
        0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2,
        0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
        0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1,
        0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
        0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192,
        0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
        0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1,
        0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
        0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151,
        0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
        0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132,
        0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
        0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312,
        0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
        0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371,
        0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
        0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1,
        0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
        0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2,
        0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
        0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291,
        0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
        0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2,
        0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
        0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252,
        0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
        0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231,
        0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202};

uint16_t Crc::Crc16_8005(uint8_t *buf, int len, uint16_t precrc)
{
    return Crc16(crc16_8005, buf, len, precrc, true, true, 0);
}

const uint32_t Crc::crc32_table[256] =
    {
        0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9,
        0x130476dc, 0x17c56b6b, 0x1a864db2, 0x1e475005,
        0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
        0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd,
        0x4c11db70, 0x48d0c6c7, 0x4593e01e, 0x4152fda9,
        0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
        0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011,
        0x791d4014, 0x7ddc5da3, 0x709f7b7a, 0x745e66cd,
        0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
        0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5,
        0xbe2b5b58, 0xbaea46ef, 0xb7a96036, 0xb3687d81,
        0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
        0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49,
        0xc7361b4c, 0xc3f706fb, 0xceb42022, 0xca753d95,
        0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
        0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d,
        0x34867077, 0x30476dc0, 0x3d044b19, 0x39c556ae,
        0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
        0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16,
        0x018aeb13, 0x054bf6a4, 0x0808d07d, 0x0cc9cdca,
        0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
        0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02,
        0x5e9f46bf, 0x5a5e5b08, 0x571d7dd1, 0x53dc6066,
        0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
        0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e,
        0xbfa1b04b, 0xbb60adfc, 0xb6238b25, 0xb2e29692,
        0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
        0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a,
        0xe0b41de7, 0xe4750050, 0xe9362689, 0xedf73b3e,
        0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
        0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686,
        0xd5b88683, 0xd1799b34, 0xdc3abded, 0xd8fba05a,
        0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
        0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb,
        0x4f040d56, 0x4bc510e1, 0x46863638, 0x42472b8f,
        0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
        0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47,
        0x36194d42, 0x32d850f5, 0x3f9b762c, 0x3b5a6b9b,
        0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
        0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623,
        0xf12f560e, 0xf5ee4bb9, 0xf8ad6d60, 0xfc6c70d7,
        0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
        0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f,
        0xc423cd6a, 0xc0e2d0dd, 0xcda1f604, 0xc960ebb3,
        0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
        0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b,
        0x9b3660c6, 0x9ff77d71, 0x92b45ba8, 0x9675461f,
        0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
        0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640,
        0x4e8ee645, 0x4a4ffbf2, 0x470cdd2b, 0x43cdc09c,
        0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
        0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24,
        0x119b4be9, 0x155a565e, 0x18197087, 0x1cd86d30,
        0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
        0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088,
        0x2497d08d, 0x2056cd3a, 0x2d15ebe3, 0x29d4f654,
        0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
        0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c,
        0xe3a1cbc1, 0xe760d676, 0xea23f0af, 0xeee2ed18,
        0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
        0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0,
        0x9abc8bd5, 0x9e7d9662, 0x933eb0bb, 0x97ffad0c,
        0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
        0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4};

uint32_t Crc::Crc32(uint8_t *buf, int len, uint32_t precrc)
{
    uint32_t crc = precrc;
    for (int index = 0; index < len; index++)
    {
        crc = ((crc << 8) ^ crc32_table[((crc >> 24) ^ buf[index]) & 255]);
    }
    return (crc);
}

int Exec::Run(const char *cmd, char *outbuf, int len)
{
    auto pipe = popen(cmd, "r");
    if (!pipe)
    {
        return -1;
    }
    char *f = fgets(outbuf, len + 1, pipe);
    pclose(pipe);
    if (f == nullptr)
    {
        return -1;
    }
    int s = strlen(outbuf);
    if (outbuf[s - 1] == '\n')
    {
        outbuf[s - 1] = '\0';
        s--;
    }
    return s;
}

void Exec::CopyFile(const char *src, const char *dst)
{
    int srcfd = open(src, O_RDONLY);
    if (srcfd < 0)
    {
        throw std::runtime_error(FmtException("Can't open %s to read", src));
    }
    int dstfd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IWUSR | S_IRUSR);
    if (dstfd < 0)
    {
        throw std::runtime_error(FmtException("Can't open %s to write", dst));
    }
    uint8_t buf[1024];
    while (1)
    {
        ssize_t rd = read(srcfd, &buf[0], sizeof(buf));
        if (rd < 0)
        {
            close(srcfd);
            close(dstfd);
            throw std::runtime_error(FmtException("Read %s error", src));
        }
        if (rd == 0)
            break;
        ssize_t wr = write(dstfd, &buf[0], rd);
        if (rd != wr)
        {
            close(srcfd);
            close(dstfd);
            throw std::runtime_error(FmtException("Write %s error", dst));
        }
    }
    fdatasync(dstfd);
    close(srcfd);
    close(dstfd);
}

bool Exec::FileExists(const char *filename)
{
    struct stat fileStat;
    if (stat(filename, &fileStat))
    {
        return false;
    }
    if (!S_ISREG(fileStat.st_mode))
    {
        return false;
    }
    return true;
}

bool Exec::DirExists(const char *dirname)
{
    struct stat fileStat;
    if (stat(dirname, &fileStat))
    {
        return false;
    }
    if (!S_ISDIR(fileStat.st_mode))
    {
        return false;
    }
    return true;
}

int Exec::Shell(const char *fmt, ...)
{
    char buf[256];
    va_list args;
    va_start(args, fmt);
    int len = vsnprintf(buf, 256, fmt, args);
    va_end(args);
    if (len == 256)
    {
        throw std::out_of_range(FmtException("Shell command is too long:%s", buf));
    }
    return system(buf);
}

uint8_t Time::monthday[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

void Time::PrintBootTime()
{
    struct timespec _CLOCK_BOOTTIME;
    clock_gettime(CLOCK_BOOTTIME, &_CLOCK_BOOTTIME);
    printf("[%ld.%09ld]\n", _CLOCK_BOOTTIME.tv_sec, _CLOCK_BOOTTIME.tv_nsec);
}

long Time::Interval()
{
    static struct timespec start = {0, 0};
    static struct timespec end;
    clock_gettime(CLOCK_BOOTTIME, &end);
    long ms = (end.tv_sec - start.tv_sec) * 1000;
    if (end.tv_nsec < start.tv_nsec)
    {
        ms += (end.tv_nsec + 1000000000 - start.tv_nsec) / 1000000 - 1000;
    }
    else
    {
        ms += (end.tv_nsec - start.tv_nsec) / 1000000;
    }
    start = end;
    return ms;
}

extern time_t GetTime(time_t *);
time_t Time::GetLocalTime(struct tm &stm)
{
    time_t t = GetTime(nullptr);
    localtime_r(&t, &stm);
    return t;
}

time_t Time::SetLocalTime(struct tm &stm)
{
    Exec::Shell("date '%d-%d-%d %d:%d:%d'",
                // Busybox command 'date', FMT: YYYY-MM-DD hh:mm[:ss]. Do not support microseconds.
                stm.tm_year + 1900, stm.tm_mon + 1, stm.tm_mday, stm.tm_hour, stm.tm_min, stm.tm_sec);
    auto t = mktime(&stm);
    auto newt = GetTime(nullptr);
    auto sec = newt - t;
    if (sec < 0 || sec > 3)
    {
        return -1;
    }
    return newt;
}

bool Time::IsTmValid(struct tm &stm)
{
    if (stm.tm_mday < 1 ||
        stm.tm_mon < 0 || stm.tm_mon > 11 ||
        stm.tm_year < 101 || stm.tm_year > 200 ||
        stm.tm_hour < 0 || stm.tm_hour > 23 ||
        stm.tm_min < 0 || stm.tm_min > 59 ||
        stm.tm_sec < 0 || stm.tm_sec > 59)
    {
        return false;
    }
    uint8_t md = monthday[stm.tm_mon];
    if (stm.tm_mon == 1 && ((stm.tm_year + 1900) % 4) == 0) // Feb of leap year
    {
        md++;
    }
    return (stm.tm_mday <= md);
}

int Time::SleepMs(long msec)
{
    struct timespec req;
    struct timespec rem;
    if (msec < 0)
    {
        errno = EINVAL;
        return -1;
    }
    rem.tv_sec = msec / 1000;
    rem.tv_nsec = (msec % 1000) * 1000000;
    int res;
    do
    {
        req = rem;
        res = nanosleep(&req, &rem);
    } while (res /* && errno == EINTR*/);
    return res;
}

int64_t Time::TimevalSubtract(struct timeval *x, struct timeval *y)
{
    int64_t xt = x->tv_sec * 1000000 + x->tv_usec;
    int64_t yt = y->tv_sec * 1000000 + y->tv_usec;
    return xt - yt;
}

uint8_t *Time::PutLocalTm(time_t t, uint8_t *p)
{
    struct tm tp;
    if (localtime_r(&t, &tp) != &tp)
    {
        memset(p, 0, 7);
        return p + 7;
    }
    *p++ = tp.tm_mday;
    *p++ = tp.tm_mon + 1;
    int year = tp.tm_year + 1900;
    *p++ = year / 0x100;
    *p++ = year & 0xFF;
    *p++ = tp.tm_hour;
    *p++ = tp.tm_min;
    *p++ = tp.tm_sec;
    return p;
}

void Time::ClearTm(struct tm *tp)
{
    tp->tm_mday = 1;
    tp->tm_mon = 0;
    tp->tm_year = 1970 - 1900;
    tp->tm_hour = 0;
    tp->tm_min = 0;
    tp->tm_sec = 0;
}

char *Time::ParseTimeToLocalStr(struct timeval *t, char *p)
{
    struct tm tp;
    if (localtime_r(&(t->tv_sec), &tp) != &tp)
    {
        ClearTm(&tp);
    }
    int len = sprintf(p, "%02d/%02d/%d %2d:%02d:%02d.%03d",
                      tp.tm_mday, tp.tm_mon + 1, tp.tm_year + 1900, tp.tm_hour, tp.tm_min, tp.tm_sec, t->tv_usec / 1000);
    return p + len;
}

char *Time::ParseTimeToLocalStr(time_t t, char *p)
{
    struct tm tp;
    if (localtime_r(&t, &tp) != &tp)
    {
        ClearTm(&tp);
    }
    int len = sprintf(p, "%2d/%02d/%d %2d:%02d:%02d",
                      tp.tm_mday, tp.tm_mon + 1, tp.tm_year + 1900, tp.tm_hour, tp.tm_min, tp.tm_sec);
    return p + len;
}

/*
time_t Time::ParseLocalStrToTm(char *pbuf)
{
    struct tm tp;
    int len = sscanf(pbuf, "%d/%d/%d %d:%d:%d",
                     &tp.tm_mday, &tp.tm_mon, &tp.tm_year, &tp.tm_hour, &tp.tm_min, &tp.tm_sec);
    if (len != 6)
    {
        return -1;
    }
    tp.tm_mon--;
    tp.tm_year -= 1900;
    tp.tm_isdst = -1;
    return mktime(&tp);
}
*/

void BitOffset::SetBit(uint8_t *buf, int bitOffset)
{
    uint8_t *p = buf + bitOffset / 8;
    uint8_t b = MASK_BIT[7 - (bitOffset & 7)];
    *p |= b;
}

void BitOffset::ClrBit(uint8_t *buf, int bitOffset)
{
    uint8_t *p = buf + bitOffset / 8;
    uint8_t b = MASK_BIT[7 - (bitOffset & 7)];
    *p &= ~b;
}

bool BitOffset::GetBit(uint8_t *buf, int bitOffset)
{
    uint8_t *p = buf + bitOffset / 8;
    uint8_t b = MASK_BIT[7 - (bitOffset & 7)];
    return (*p & b) != 0;
}

Bits::Bits(int size)
    : size(size)
{
    data.resize((size + 7) / 8);
    ClrAll();
}

void Bits::Init(int size)
{
    this->size = size;
    data.resize((size + 7) / 8);
    ClrAll();
}

Bits::~Bits()
{
}

void Bits::SetBit(int bitOffset)
{
    Check(bitOffset);
    BitOffset::SetBit(data.data(), bitOffset);
}

void Bits::ClrBit(int bitOffset)
{
    Check(bitOffset);
    BitOffset::ClrBit(data.data(), bitOffset);
}

void Bits::ClrAll()
{
    data.assign(data.size(), 0);
}

bool Bits::GetBit(int bitOffset)
{
    Check(bitOffset);
    return BitOffset::GetBit(data.data(), bitOffset);
}

std::string Bits::ToString()
{
    char buf[1024];
    int len = 0;
    for (int i = 0; i < size && i < 256; i++)
    {
        if (BitOffset::GetBit(data.data(), i))
        {
            len += sprintf(buf + len, (len == 0) ? "%d" : ",%d", i);
        }
    }
    return (len == 0) ? " " : std::string{buf};
}

void Bits::Check(int bitOffset)
{
    if (bitOffset >= size)
    {
        throw std::out_of_range(FmtException("Bits(size=%d):out_of_range: bit[%d]", size, bitOffset));
    }
}

int Bits::GetMaxBit()
{
    int max = -1;
    for (int i = 0; i < size; i++)
    {
        if (BitOffset::GetBit(data.data(), i))
        {
            max = i;
        }
    }
    return max;
}

void Bits::Clone(Bits &v)
{
    size = v.Size();
    data = v.Data();
}

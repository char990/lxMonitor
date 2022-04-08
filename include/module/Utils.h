#pragma once

#include <cstdint>
#include <ctime>
#include <string>
#include <cstring>
#include <vector>
#include <sys/time.h>

#define GIGA(x) (x * 1024 * 1024 * 1024ULL)
#define MEGA(x) (x * 1024 * 1024UL)
#define KILO(x) (x * 1024UL)
#define SEC_PER_DAY (24 * 3600)

namespace Utils
{
    enum class STATE3
    {
        S3_NA = -1,
        S3_0 = 0,
        S3_1 = 1
    };

    enum class STATE5
    {
        S5_NA = -1,
        S5_0 = 0, // logic 0
        S5_1 = 1, // logic 1
        S5_F = 2, // Falling edge
        S5_R = 3  // Rising edge
    };
    class State5
    {
    public:
        State5() { pre_v = value = STATE5::S5_NA; };
        State5(STATE5 _v) : value(_v), pre_v(_v){};
        void Init(STATE5 _v) { pre_v = value = _v; };
        void Set()
        {
            pre_v = value;
            if (value == STATE5::S5_NA || value == STATE5::S5_0 || value == STATE5::S5_F)
            {
                value = STATE5::S5_R;
            }
        };
        void Clr()
        {
            pre_v = value;
            if (value == STATE5::S5_NA || value == STATE5::S5_1 || value == STATE5::S5_R)
            {
                value = STATE5::S5_F;
            }
        };
        void ClearEdge()
        {
            if (value == STATE5::S5_R)
            {
                value = STATE5::S5_1;
            }
            else if (value == STATE5::S5_F)
            {
                value = STATE5::S5_0;
            }
        };

        void ClearRising()
        {
            if (value == STATE5::S5_R)
            {
                value = STATE5::S5_1;
            }
        };
        void ClearFalling()
        {
            if (value == STATE5::S5_F)
            {
                value = STATE5::S5_0;
            }
        };

        bool HasEdge() { return value == STATE5::S5_R || value == STATE5::S5_F; };
        bool IsRising() { return value == STATE5::S5_R; };
        bool IsHigh() { return value == STATE5::S5_R || value == STATE5::S5_1; };
        bool IsFalling() { return value == STATE5::S5_F; };
        bool IsLow() { return value == STATE5::S5_F || value == STATE5::S5_0; };
        bool IsValid() { return value != STATE5::S5_NA; };
        STATE5 Value() { return value; };
        STATE5 PreV() { return pre_v; };
        const char *State()
        {
            if (value == STATE5::S5_0)
                return "Low";
            else if (value == STATE5::S5_1)
                return "High";
            else if (value == STATE5::S5_F)
                return "Falling";
            else if (value == STATE5::S5_R)
                return "Rising";
            return "N/A";
        }

    protected:
        STATE5 value;
        STATE5 pre_v;
    };

    extern const uint32_t MASK_BIT[32];

    class Check
    {
    public:
        /// \brief  Check if input is a legal Hex string
        /// \return 0:success
        static int HexStr(uint8_t *frm, int len);

        /// \brief  Check if input is text(0x20-0x7F)
        /// \return 0:success
        static int Text(uint8_t *frm, int len);
    };

    class Cnvt
    {
    public:
        /// \brief  Reverse a uint8_t Example: 0x1A => 0x85
        static uint8_t Reverse(uint8_t n);

        /// \brief  parse 2 Asc to 1 uint8_t Hex. Example: "1F" => 0x1F
        /// \return     int: 0-255:success, -1:failed, there is invalid chars
        static int ParseToU8(const char *p);

        /// \brief  parse 2 Asc to 1 Hex. Example: "1F....." => 0x1F......
        /// \param      src : ascii buffer
        /// \param      dst : hex buffer
        /// \param      srclen : ascii len ( = hex_len *2)
        /// \return     int: 0:success, -1:failed, there is invalid chars
        static int ParseToU8(const char *src, uint8_t *dst, int srclen);

        /// \brief  parse Asc to uint16_t hex. Example: "1F0A" => 0x1F0A
        /// \param      src : ascii buffer
        /// \return     int: >0:success, -1:failed, there is invalid chars
        static int ParseToU16(const char *src);

        /// \brief  parse Asc to uint32_t hex. Example: "1F0A3456" => 0x1F0A3456
        /// \param      src : ascii buffer
        /// \return     int64_t: >0:success, -1:failed, there is invalid chars
        static int64_t ParseToU32(const char *src);

        /// \brief  parse uint8_t to 2 Asc. Example: 0x1F => "1F"
        /// \return     next byte of dst
        static char *ParseToAsc(uint8_t h, char *dst);

        /// \brief  parse uint8_t array to 2 Asc no '\0' attached. Example: 0x1F 0x2A 0x3E ... => "1F2A3E......"
        /// \param      src : hex buffer
        /// \param      dst : ascii buffer
        /// \param      srclen : hex len ( = asc_len / 2)
        /// \return     next byte of dst
        static char *ParseToAsc(uint8_t *src, char *dst, int srclen);

        /// \brief  parse uint8_t array to 2 Asc with '\0' attached. Example: 0x1F 0x2A 0x3E ... => "1F2A3E......\0"
        /// \param      src : hex buffer
        /// \param      dst : ascii buffer
        /// \param      srclen : hex len ( = asc_len / 2)
        /// \return     next byte of dst
        static char *ParseToStr(uint8_t *src, char *dst, int srclen);

        /// \brief  parse uint16_t to 4 Asc. Example: 0x1F09 => "1F09"
        /// \return     next byte of dst
        static char *ParseU16ToAsc(uint16_t h, char *dst);

        /// \brief  parse uint32_t to 4 Asc. Example: 0x1F09 => "1F09"
        /// \return     next byte of dst
        static char *ParseU32ToAsc(uint32_t h, char *dst);

        /// \brief	convert int string to array, "2,3,100" => {0x02,0x03,0x64}
        ///			If number is less than min or greater than max, just return
        /// \param	src: input
        /// \param	srcmax: max src size to be converted
        /// \param	dst: output
        /// \param	min: min
        /// \param	max: max
        /// \return numbers converted
        static int GetIntArray(const char *src, int srcmax, int *dst, int min, int max);

        /// \brief Convert 2 bytes uint8_t to uint16_t
        static uint16_t GetU16(uint8_t *p);
        /// \brief Put uint16_t to uint8_t *
        static uint8_t *PutU16(uint16_t v, uint8_t *p);

        /// \brief Convert 4 bytes uint8_t to uint32_t
        static uint32_t GetU32(uint8_t *p);
        /// \brief Put uint32_t to uint8_t *
        static uint8_t *PutU32(uint32_t v, uint8_t *p);

        /// \brief  split a string
        static void split(const std::string &s, std::vector<std::string> &tokens, const std::string &delimiters = " ");

        /// \brief  Swap byte of uint16_t
        static uint16_t SwapU16(uint16_t v);

        /// \brief  Get int16_t from uint8_t array, [0]=high byte,[1]=low byte
        static int16_t GetS16hl(uint8_t *p);
        /// \brief  Get int16_t from uint8_t array, [0]=low byte,[1]=high byte
        static int16_t GetS16lh(uint8_t *p);
    };

    class Crc
    {
    public:
// online crc calculator http://www.sunshine2k.de/coding/javascript/crc/crc_js.html
// If combines the data packet with the crc to be a new data packet,
// the crc of the new data packet will be 0

// CRC8-CCITT - x^8+x^2+x+1
/*
        This CRC can be specified as:
        Width  : 8
        Poly   : 0x07
        Init   : 0
        RefIn  : false
        RefOut : false
        XorOut : 0
        */
#define PRE_CRC8 0
        static const uint8_t crc8_table[256];
        static uint8_t Crc8(uint8_t *buf, int len, uint8_t precrc = PRE_CRC8);

/// \brief Crc16
#define PRE_CRC16 0

        static uint16_t Crc16(const uint16_t *table, uint8_t *buf, int len,
                              uint16_t init, bool refIn, bool refOut, uint16_t xorOut);

        // CRC16-CCITT polynomial X^16 + X^12 + X^5 + X^0 0x11021
        /*
        This CRC can be specified as:
        Width  : 16
        Poly   : 0x1021
        Init   : 0
        RefIn  : false
        RefOut : false
        XorOut : 0
        */
        static const uint16_t crc16_1021[256];
        static uint16_t Crc16_1021(uint8_t *buf, int len, uint16_t precrc = PRE_CRC16);

        // CRC16-IBM polynomial X^15 + X^2 + X^0 0x8005
        /* !!! Not test
        This CRC can be specified as:
        Width  : 16
        Poly   : 0x8005
        Init   : 0
        RefIn  : true
        RefOut : true
        XorOut : 0
        */
        static const uint16_t crc16_8005[256];
        static uint16_t Crc16_8005(uint8_t *buf, int len, uint16_t precrc = PRE_CRC16);

/*
        https://android.googlesource.com/toolchain/binutils/+/53b6ed3bceea971857c996b6dcb96de96b99335f/binutils-2.19/libiberty/crc32.c
        This CRC can be specified as:
        Width  : 32
        Poly   : 0x04c11db7
        Init   : 0xffffffff
        RefIn  : false
        RefOut : false
        XorOut : 0
        This differs from the "standard" CRC-32 algorithm in that the values
        are not reflected, and there is no final XOR value.  These differences
        make it easy to compose the values of multiple blocks.
        */
#define PRE_CRC32 0xFFFFFFFF
        static const uint32_t crc32_table[256];

        /*! \brief CRC32 function. Poly   : 0x04C11DB7
         *
         * \param buf   input array
         * \param len   how many bytes
         *
         * \return crc32
         */
        static uint32_t Crc32(uint8_t *buf, int len, uint32_t precrc = PRE_CRC32);
    };

    class Exec
    {
    public:
        /// \brief      Run a command and save the stdout to outbuf
        /// \param      cmd:command
        /// \param      outbuf:output
        /// \param      len: get first 'len' chars of stdout(output buffer size sould be len+1, a '\0' attched to the end)
        /// \return     -1:failed; >=0:strlen(tail '\n' was trimmed)
        static int Run(const char *cmd, char *outbuf, int len);

        /// \brief      Copy a file
        /// \param      src:
        /// \param      dst:
        static void CopyFile(const char *src, const char *dst);

        /// \brief      Check if a file/dir exists
        static bool FileExists(const char *path);
        static bool DirExists(const char *path);

        // run shell command
        static int Shell(const char *fmt, ...);
    };

    class Time
    {
    public:
        static uint8_t monthday[12];
        /// \brief      Print time
        static void PrintBootTime();

        /// \brief      Get interval from last Interval() called
        /// \return     interval of ms
        long Interval();

        /// \brief      get localtime to stm
        /// \return     time_t
        static time_t GetLocalTime(struct tm &stm);

        /// \brief      set system time from localtime-stm
        /// \return     time_t of stm, -1:failed
        static time_t SetLocalTime(struct tm &stm);

        /// \brief      check if a broken time is valid
        ///             from 1/1/2001 0:00:00
        static bool IsTmValid(struct tm &stm);

        /// \brief      sleep ms
        static int SleepMs(long msec);

        /// \brief      subtract y from x
        /// \return     us
        static int64_t TimevalSubtract(struct timeval *x, struct timeval *y);

        /// \brief Parse time_t to localtime and wrtie to uint8_t *Tm
        static uint8_t *PutLocalTm(time_t t, uint8_t *tm);

        /// \brief Set tp as 1/1/1970 0:00:00
        static void ClearTm(struct tm *tp);

        /// \brief  Parse time_t to localtime string and wrtie to char *pbuf
        ///         string format: d/M/yyyy h:mm:ss
        /// \return next byte of output buf
        static char *ParseTimeToLocalStr(time_t t, char *pbuf);

        /// \brief  Parse timeval to localtime string and wrtie to char *pbuf
        ///         string format: d/M/yyyy h:mm:ss.mmm
        /// \return next byte of output buf
        static char *ParseTimeToLocalStr(struct timeval *t, char *p);

        /// \brief  Parse localtime string to tm_t
        /// !!! NOTE !!! When the time is between the override hour of 2:00-3:00, can't get correct time_t, so don not use this function
        ///         string format: d/M/yyyy h:mm:ss
        /// \retunr -1:fail
        // static time_t ParseLocalStrToTm(char *pbuf);
    };

    /// \brief  Set/Clr bit in uint8_t *buf
    /// bifOffset is from [0:7](0), ..., [0:0](7), [1:7](8), ..., [1:0](15), ...
    class BitOffset
    {
    public:
        static void SetBit(uint8_t *buf, int bitOffset);
        static void ClrBit(uint8_t *buf, int bitOffset);
        static bool GetBit(uint8_t *buf, int bitOffset);
    };

    class Bits
    {
    public:
        Bits(int size);
        Bits(){};
        ~Bits();
        void Init(int size);
        int Size() { return size; };
        void SetBit(int bitOffset);
        void ClrBit(int bitOffset);
        void ClrAll();
        bool GetBit(int bitOffset);
        int GetMaxBit();
        std::string ToString();
        void Clone(Bits &v);
        std::vector<uint8_t> &Data() { return data; };

    private:
        int size{0};
        std::vector<uint8_t> data;
        void Check(int bitOffset);
    };

    // Only for byte type: uint8_t/char
    template <typename T>
    T *CharCpy(T *dst, const char *from, int n)
    {
        if (from == nullptr || n <= 0)
        {
            return dst;
        }
        if (sizeof(T) != sizeof(char))
        {
            throw "CharCpy: sizeof(T)!=sizeof(char)";
        }
        int len = strlen(from);
        if (len > n)
            len = n;
        memcpy(dst, from, len);
        dst += len;
        *dst = '\0';
        return dst;
    }

    template <typename T, std::size_t N>
    constexpr std::size_t countof(T const (&)[N]) noexcept
    {
        return N;
    }

    /// \brief  More String Functions
    class StrFn
    {
    public:
        static std::vector<std::string> Split(const std::string& i_str, const std::string& i_delim);
        static int vsPrint(std::vector<std::string> *vs);
    };
    
} // namespace Utils

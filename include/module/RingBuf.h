#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

class RingBuf
{
public:
    RingBuf(int s) : size(s)
    {
        buf = new uint8_t[s];
        bufEnd = buf + s;
        Reset();
    };
    ~RingBuf()
    {
        delete[] buf;
    }

    int Push(const uint8_t *inbuf, int len)
    {
        if (len <= 0)
        {
            return 0;
        }
        if(len > Vacancy())
        {
            len = Vacancy();
        } 
        int size1 = bufEnd - pPush;
        if (size1 > len)
        {
            memcpy(pPush, inbuf, len);
            pPush += len;
        }
        else
        {
            memcpy(pPush, inbuf, size1);
            memcpy(buf, inbuf + size1, len - size1);
            pPush = buf + len - size1;
        }
        cnt += len;
        return len;
    }

    int Pop(uint8_t *outbuf, int len)
    {
        if (cnt == 0 || len == 0)
        {
            return 0;
        }
        if (len > cnt)
        {
            len = cnt;
        }
        int size1 = bufEnd - pPop;
        if (len > size1)
        {
            memcpy(outbuf, pPop, size1);
            memcpy(outbuf + size1, buf, len - size1);
            pPop = buf + len - size1;
        }
        else
        {
            memcpy(outbuf, pPop, len);
            pPop += len;
        }
        cnt -= len;
        return len;
    }

    /// \brief the number of elements that can be held in currently allocated storage
    int Size() { return size; };

    /// \brief the number of elements
    int Cnt() { return cnt; };

    /// \brief the number of elements of vacancy
    int Vacancy() { return size - cnt; };

    void Reset()
    {
        cnt = 0;
        pPush = pPop = buf;
    };

    std::string ToString()
    {
        char str[256];
        sprintf(str, "\nthis=%p\nsize=%d\ncnt=%d\nvacancy=%d\npPush=%p\npPop=%p\nbuf=%p\nbufEnd=%p\n",
                this, size, cnt, Vacancy(), pPush, pPop, buf, bufEnd);
        return std::string(str);
    };

private:
    int size;
    int cnt;
    uint8_t *pPush;
    uint8_t *pPop;
    uint8_t *buf;
    uint8_t *bufEnd;
};

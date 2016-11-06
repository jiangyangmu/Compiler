#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
using namespace std;

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

class StringBuf
{
   public:
    explicit StringBuf(const char *data)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        char *data2 = new char[count + 1];

        copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    StringBuf(const char *data, size_t n)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        count = (count > n) ? n : count;
        char *data2 = new char[count + 1];

        copy(data, data + count, data2);
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    ~StringBuf() { delete[] begin; }
    char peak(size_t offset = 0)
    {
        if (now + offset < end)
            return now[offset];
        else
            return '\0';
    }
    void pop(size_t count = 1)
    {
        const char *now2 = now + count;
        if (now2 < now || now2 > end)
            now = end;
        else
            now = now2;
    }
    bool empty() { return now == end; }
    size_t size() { return end - now; }
    const char *data() { return now; }
   private:
    const char *begin, *end, *now;
};

class StringRef
{
   public:
    StringRef() { begin = end = nullptr; }
    explicit StringRef(const char *data)
    {
        assert(data != nullptr);

        begin = data;
        end = data + strlen(begin);
    }
    // fast or safety ? fast
    StringRef(const char *data, size_t n)
    {
        assert(data != nullptr);

        begin = data;
        end = data + n;
    }

    friend bool operator==(const StringRef &s1, const char *s2)
    {
        assert(s2 != nullptr);

        const char *p1 = s1.begin, *p2 = s2;
        while (p1 != s1.end && *p2 != '\0')
        {
            if (*p1 != *p2)
                return false;
            ++p1, ++p2;
        }
        return p1 == s1.end && *p2 == '\0';
    }
    friend bool operator==(const char *s1, const StringRef &s2)
    {
        return s2 == s1;
    }
    friend ostream &operator<<(ostream &o, const StringRef &s)
    {
        copy(s.begin, s.end, ostream_iterator<char>(o));
        return o;
    }

   private:
    const char *begin, *end;
};


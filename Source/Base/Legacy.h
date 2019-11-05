#pragma once

// Deprecated code.

#include <iostream>

#include "ErrorHandling.h"

class StringRef
{
public:
    StringRef()
    {
        begin_ = end_ = "";
    }
    StringRef(const char *data)
    {
        ASSERT(data != nullptr);

        begin_ = data;
        end_ = data + strlen(begin_);
    }
    // fast or safety ? fast
    StringRef(const char *data, size_t n)
    {
        ASSERT(data != nullptr);

        begin_ = data;
        end_ = data + n;
    }
    StringRef &operator=(const char *data)
    {
        return (*this = StringRef(data));
    }
    StringRef(const StringRef & other)
        : begin_(other.begin_), end_(other.end_)
    {
    }
    StringRef(StringRef && other)
        : begin_(other.begin_), end_(other.end_)
    {
    }
    StringRef & operator = (const StringRef & other)
    {
        begin_ = other.begin_;
        end_ = other.end_;
        return *this;
    }
    StringRef & operator = (StringRef && other)
    {
        begin_ = other.begin_;
        end_ = other.end_;
        return *this;
    }
    StringRef(const std::string & s) : begin_(s.data()), end_(s.data() + s.length()) {}
    ~StringRef() {}

    void clear()
    {
        begin_ = end_ = "";
    }

    bool empty() const
    {
        return begin_ == end_;
    }

    size_t size() const
    {
        return end_ - begin_;
    }

    char front() const
    {
        ASSERT(!empty());
        return *begin_;
    }

    char back() const
    {
        ASSERT(!empty());
        return *(end_ - 1);
    }

    std::string toString() const
    {
        std::string s;
        if (end_ > begin_)
            s.assign(begin_, end_ - begin_);
        return s;
    }

    const char *data() const
    {
        return begin_;
    }

    const char *begin() const
    {
        return begin_;
    }

    const char *end() const
    {
        return end_;
    }

    char operator[](size_t offset) const
    {
        ASSERT((begin_ + offset) < end_);
        return begin_[offset];
    }

    bool operator==(const StringRef &other) const
    {
        const char *p1 = begin_, *p2 = other.begin_;
        while (p1 != end_ && p2 != other.end_)
        {
            if (*p1 != *p2)
                break;
            ++p1, ++p2;
        }
        return (p1 == end_) && (p2 == other.end_);
    }
    bool operator!=(const StringRef &other) const
    {
        return !(*this == other);
    }

    friend bool operator==(const StringRef &s1, const char *s2)
    {
        ASSERT(s2 != nullptr);

        const char *p1 = s1.begin_, *p2 = s2;
        while (p1 != s1.end_ && *p2 != '\0')
        {
            if (*p1 != *p2)
                return false;
            ++p1, ++p2;
        }
        return p1 == s1.end_ && *p2 == '\0';
    }
    friend bool operator==(const char *s1, const StringRef &s2)
    {
        return s2 == s1;
    }
    friend bool operator!=(const StringRef &s1, const char *s2)
    {
        return !(s1 == s2);
    }
    friend bool operator!=(const char *s1, const StringRef &s2)
    {
        return !(s2 == s1);
    }
    friend std::ostream &operator<<(std::ostream &o, const StringRef &s)
    {
        std::copy(s.begin_, s.end_, std::ostream_iterator<char>(o));
        return o;
    }

private:
    const char *begin_, *end_;
};

using String = std::string;

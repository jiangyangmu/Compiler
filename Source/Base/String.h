#pragma once

#include <cstring>
#include <string>
#include <iterator>

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
void SetBytes(char * bytes, char value, size_t count);
void CopyBytes(const char * from, char * to, size_t count);

class ByteArray
{
public:
    // create, destroy
    ByteArray();
    ByteArray(const ByteArray & other);
    ByteArray(ByteArray && other);
    ByteArray(const char * data, size_t size);
    ~ByteArray();

    // read, write
    void            PushBack(char c);
    char            PopBack();
    void            Clear();
    char            First() const;
    char            Last() const;

    // size, capacity
    bool            Empty() const { return size_ == 0; }
    size_t          Size() const { return size_; }
    void            Reserve(size_t capacity);

    // raw
    char *          RawData() { return data_; }
    const char *    RawData() const { return data_; }
    // size <= capacity
    void            SetSize(size_t size);

    // copy, move
    ByteArray &     operator=(const ByteArray & other);
    ByteArray &     operator=(ByteArray && other);

    // STL interface
    const char *    begin() const { return data_; }
    const char *    end() const { return data_ ? (data_ + size_) : data_; }
    char *          begin() { return data_; }
    char *          end() { return data_ ? (data_ + size_) : data_; }
    friend std::ostream &operator<<(std::ostream & o, const ByteArray & byteArray)
    {
        std::copy(byteArray.data_, byteArray.data_ + byteArray.size_, std::ostream_iterator<char>(o));
        return o;
    }

private:
    char * data_;
    size_t size_;
    size_t capacity_;
};

class StringView
{
public:
    StringView() : begin(nullptr), end(nullptr) {}
    StringView(const StringView & other) : begin(other.begin), end(other.end) {}
    StringView(StringView && other) : begin(other.begin), end(other.end) {}
    StringView& operator = (const StringView & other) { begin = other.begin; end = other.end; }
    StringView& operator = (StringView && other) { begin = other.begin; end = other.end; }

    StringView(const char * str) : begin(str), end(str + strlen(str)) {}
    StringView(const char * str, size_t n) : begin(str), end(str + n) {}
    StringView(const std::string & str) : begin(str.data()), end(str.data() + str.length()) {}

    // query
    bool    Empty() const { return begin == end; }
    size_t  Length() const { return end - begin; }
    char    First() const
    {
        assert(!Empty());
        return *begin;
    }
    char    Last() const
    {
        assert(!Empty());
        return *(end - 1);
    }

    const char * Begin() const { return begin; }
    const char * End() const { return end; }
    char operator [] (size_t index) const
    {
        assert(index < Length());
        return begin[index];
    }

    // modify
    void    Clear() { begin = end = nullptr; }

    // compare
    bool operator < (const StringView & other) const
    {
        if (begin == other.begin)
            return end < other.end;

        const char *p1 = begin;
        const char *p2 = other.begin;

        while (p1 != end &&
               p2 != other.end &&
               *p1 == *p2)
        {
            ++p1, ++p2;
        }

        if (p1 != end)
            return p2 != end && *p1 < *p2;
        else
            return p2 != end;
    }
    bool operator == (const StringView & other) const
    {
        if (Length() != other.Length())
            return false;
        if (begin == other.begin)
            return true;

        const char *p1 = begin;
        const char *p2 = other.begin;

        while (p1 != end &&
               p2 != other.end &&
               *p1 == *p2)
        {
            ++p1, ++p2;
        }

        return (p1 == end);
    }
    bool operator != (const StringView & other) const
    {
        return !(*this == other);
    }

    // interop
    operator std::string () const { return Empty() ? std::string() : std::string(begin, end); }
    // io
    friend std::ostream &operator<<(std::ostream &o, const StringView & s)
    {
        std::copy(s.begin, s.end, std::ostream_iterator<char>(o));
        return o;
    }

private:
    const char * begin;
    const char * end;
};

// StringView RemovePrefix(StringView);
// StringView RemoveSuffix(StringView);

class ImmutableStringTable
{
public:
    static StringView Get(const std::string & str);
    static StringView Get(const char * str);
    static StringView Get(const char * str, size_t n);
};

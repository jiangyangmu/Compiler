#pragma once

#include <cassert>
#include <cstring>
#include <string>
#include <iterator>

class StringBuf
{
public:
    explicit StringBuf(const char *data)
    {
        assert(data != nullptr);

        size_t count = static_cast<size_t>(strlen(data));
        char *data2 = new char[count + 1];

        std::copy(data, data + count, stdext::make_checked_array_iterator(data2, count + 1));
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

        std::copy(data, data + count, stdext::make_checked_array_iterator(data2, count + 1));
        data2[count] = '\0';

        begin = now = data2;
        end = begin + count;
    }

    ~StringBuf()
    {
        delete[] begin;
    }
    char peak(size_t offset = 0) const
    {
        if (now + offset < end)
            return now[offset];
        else
            return '\0';
    }
    char pop1()
    {
        if (now < end)
            return *(now++);
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
    bool empty() const
    {
        return now == end;
    }
    size_t size() const
    {
        return end - now;
    }
    const char *data() const
    {
        return now;
    }

private:
    const char *begin, *end, *now;
};

class StringRef
{
public:
    StringRef()
    {
        begin_ = end_ = "";
    }
    StringRef(const char *data)
    {
        assert(data != nullptr);

        begin_ = data;
        end_ = data + strlen(begin_);
    }
    // fast or safety ? fast
    StringRef(const char *data, size_t n)
    {
        assert(data != nullptr);

        begin_ = data;
        end_ = data + n;
    }
    StringRef &operator=(const char *data)
    {
        return (*this = StringRef(data));
    }

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
        assert(!empty());
        return *begin_;
    }

    char back() const
    {
        assert(!empty());
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
        assert((begin_ + offset) < end_);
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
        assert(s2 != nullptr);

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

// StrCat
// int->str
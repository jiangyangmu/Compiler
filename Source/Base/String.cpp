#include "String.h"

#include "Integer.h"

void SetBytes(char * bytes, char value, size_t count)
{
    while (count-- > 0)
        *bytes++ = value;
}

void CopyBytes(const char * from, char * to, size_t count)
{
    while (count-- > 0)
    {
        *to++ = *from++;
    }
}

ByteArray::ByteArray() : data_(nullptr), size_(0), capacity_(0)
{
}

ByteArray::ByteArray(const ByteArray & other) : data_(nullptr), size_(0), capacity_(0)
{
    if (other.data_)
    {
        data_ = new char[other.size_];
        size_ = other.size_;
        capacity_ = other.capacity_;
        CopyBytes(other.data_, data_, other.size_);
    }
}

ByteArray::ByteArray(ByteArray && other) : data_(other.data_), size_(other.size_), capacity_(other.capacity_)
{
    other.data_ = nullptr;
    other.size_ = other.capacity_ = 0;
}

ByteArray::ByteArray(const char * data, size_t size)
{
    ASSERT(size > 0);
    data_ = new char[size];
    size_ = capacity_ = size;
    CopyBytes(data, data_, size);
}

ByteArray::~ByteArray()
{
    if (data_)
    {
        delete[] data_;
        data_ = nullptr;
        size_ = capacity_ = 0;
    }
}

void ByteArray::PushBack(char c)
{
    if (size_ == capacity_)
    {
        Reserve(Max(1ull, size_ * 2));
    }
    *(data_ + size_) = c;
    ++size_;
}

char ByteArray::PopBack()
{
    char c = Last();
    --size_;
    return c;
}

void ByteArray::Clear()
{
    if (data_)
    {
        delete[] data_;
        data_ = nullptr;
        size_ = capacity_ = 0;
    }
}

char ByteArray::First() const
{
    ASSERT(size_ > 0);
    return *data_;
}

char ByteArray::Last() const
{
    ASSERT(size_ > 0);
    return *(data_ + size_ - 1);
}

void ByteArray::Reserve(size_t capacity)
{
    if (capacity_ < capacity)
    {
        char * newData = new char[capacity];
        SetBytes(newData, 0, capacity);

        if (data_)
        {
            CopyBytes(data_, newData, size_);
            delete[] data_;
        }

        data_ = newData;
        capacity_ = capacity;
    }
}

void ByteArray::SetSize(size_t size)
{
    ASSERT(size <= capacity_);

    size_ = size;
}

ByteArray & ByteArray::operator=(ByteArray && other)
{
    this->~ByteArray();
    new (this) ByteArray(std::move(other));
    return *this;
}

ByteArray & ByteArray::operator=(const ByteArray & other)
{
    new (this) ByteArray(other);
    return *this;
}

static inline void MakeNullTerminated(containers::Array<char> & aStr)
{
    if (aStr.IsEmpty() || aStr[aStr.Count() - 1] != 0)
        aStr.Add(0);
}

String::String()
{
    MakeNullTerminated(aData);
}

String::String(const String & other)
    : aData(other.aData)
{
}

String::String(String && other)
    : aData(std::move(other.aData))
{
}

String::~String()
{
}

String::String(char * data)
    : aData((int)strlen(data), data)
{
    MakeNullTerminated(aData);
}

String::String(char * data, int length)
    : aData(length, data)
{
    MakeNullTerminated(aData);
}

String::String(char ch, int count)
    : aData(count, ch)
{
    MakeNullTerminated(aData);
}

bool String::Empty() const
{
    return aData.Count() == 1;
}

size_t String::Length() const
{
    return (size_t)aData.Count() - 1;
}

char String::First() const
{
    ASSERT(!Empty());
    return aData[0];
}

char String::Last() const
{
    ASSERT(!Empty());
    return aData[(int)Length() - 1];
}

char String::At(int index) const
{
    ASSERT(0 <= index && index < Length());
    return aData[index];
}

const char * String::RawData() const
{
    return aData.RawData();
}

String & String::Add(char c)
{
    aData.InsertAt((int)Length(), c);
    return *this;
}

String & String::Append(const String & s)
{
    aData.InsertAt((int)Length(), s.aData.RawData(), (int)s.Length());
    return *this;
}

String & String::ShrinkTo(int length)
{
    int nRemove = static_cast<int>(Length()) - length;
    if (0 < nRemove)
    {
        aData.RemoveAt(length, nRemove);
    }
    return *this;
}

void String::Clear()
{
    aData.RemoveAll();
    MakeNullTerminated(aData);
    aData.ReduceMemoryUsage();
}

String & String::operator=(String && s)
{
    this->~String();
    new (this) String(static_cast<String &&>(s));
    return *this;
}

bool String::operator!=(const String & other) const
{
    return !(*this == other);
}

bool String::operator==(const String & other) const
{
    return containers::IsEqual<char>(aData, other.aData);
}

bool String::operator<(const String & other) const
{
    const char * pLeft = aData.RawData();
    const char * pRight = other.aData.RawData();

    ASSERT(pLeft && pRight);
    while (*pLeft && *pRight && *pLeft == *pRight)
        ++pLeft, ++pRight;

    return *pLeft < *pRight;
}

String & String::operator+=(const String & s)
{
    Append(s);
    return *this;
}

String::operator std::string() const
{
    return std::string(aData.RawData(), aData.Count());
}

std::ostream & operator << (std::ostream & o, const String & s)
{
    return o.write(s.aData.RawData(), s.Length());
}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

TEST(String_API)
{
    String s0;
    String s1("hello");
    String s2("world", 5);
    String s3('!', 3);

    EXPECT_TRUE(s0.Empty());
    EXPECT_EQ(s0.Length(), 0);

    EXPECT_FALSE(s1.Empty());
    EXPECT_EQ(s1.Length(), 5);

    EXPECT_FALSE(s2.Empty());
    EXPECT_EQ(s2.Length(), 5);

    EXPECT_FALSE(s3.Empty());
    EXPECT_EQ(s3.Length(), 3);

    String s1_copy;
    s1_copy.Append(s1);
    EXPECT_EQ(s1_copy, s1);

    String hw;
    hw.Append(s1).Add(',').Append(s2).Append(s3);
    EXPECT_EQ(hw, String("hello,world!!!"));
}

#endif

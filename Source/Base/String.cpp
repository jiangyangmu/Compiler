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

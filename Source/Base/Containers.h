#pragma once

#include "../Base/Integer.h"
#include "../Base/ErrorHandling.h"
#include "../Memory/Allocate.h"

// Utils

// C++ ranged for adapter.
template <typename T>
class MirroredPtr
{
public:
    MirroredPtr() : ptr(nullptr) {}
    MirroredPtr(const MirroredPtr & other) : ptr(other.ptr) {}
    MirroredPtr(MirroredPtr && other) : ptr(other.ptr) {}
    MirroredPtr(T * ptr) : ptr(ptr) {}

    T & operator* () const { return *(ptr - 1); }
    MirroredPtr & operator++ () { --ptr; return *this; }
    MirroredPtr operator++ (int) { MirroredPtr tmp = *this; --ptr; return tmp; }
    MirroredPtr & operator-- () { ++ptr; return *this; }
    MirroredPtr operator-- (int) { MirroredPtr tmp = *this; ++ptr; return tmp; }

    bool operator== (const MirroredPtr & other) const { return ptr == other.ptr; }
    bool operator!= (const MirroredPtr & other) const { return ptr != other.ptr; }

private:
    T * ptr;
};
template <typename T>
class IntegerIterator
{
public:
    IntegerIterator() : intVal(0) {}
    IntegerIterator(const IntegerIterator & other) : intVal(other.intVal) {}
    IntegerIterator(IntegerIterator && other) : intVal(other.intVal) {}

    IntegerIterator(T intVal) : intVal(intVal) {}

    T operator* () const { return intVal; }
    IntegerIterator & operator++ () { ++intVal; return *this; }
    IntegerIterator operator++ (int) { IntegerIterator tmp = *this; ++intVal; return tmp; }
    IntegerIterator & operator-- () { --intVal; return *this; }
    IntegerIterator operator-- (int) { IntegerIterator tmp = *this; --intVal; return tmp; }

    bool operator== (const IntegerIterator & other) const { return intVal == other.intVal; }
    bool operator!= (const IntegerIterator & other) const { return intVal != other.intVal; }

private:
    T intVal;
};
using IntIterator = IntegerIterator<int>;
template <typename T>
class MirroredIntegerIterator
{
public:
    MirroredIntegerIterator() : intVal(0) {}
    MirroredIntegerIterator(const MirroredIntegerIterator & other) : intVal(other.intVal) {}
    MirroredIntegerIterator(MirroredIntegerIterator && other) : intVal(other.intVal) {}

    MirroredIntegerIterator(T intVal) : intVal(intVal) {}

    T operator* () const { return intVal - 1; }
    MirroredIntegerIterator & operator++ () { --intVal; return *this; }
    MirroredIntegerIterator operator++ (int) { MirroredIntegerIterator tmp = *this; --intVal; return tmp; }
    MirroredIntegerIterator & operator-- () { ++intVal; return *this; }
    MirroredIntegerIterator operator-- (int) { MirroredIntegerIterator tmp = *this; ++intVal; return tmp; }

    bool operator== (const MirroredIntegerIterator & other) const { return intVal == other.intVal; }
    bool operator!= (const MirroredIntegerIterator & other) const { return intVal != other.intVal; }

private:
    T intVal;
};
using MirroredIntIterator = MirroredIntegerIterator<int>;
template <typename TIterator>
class IteratorPair
{
public:
    IteratorPair() : beginIt(), endIt() {}
    IteratorPair(const IteratorPair & other) : beginIt(other.beginIt), endIt(other.endIt) {}
    IteratorPair(IteratorPair && other) : beginIt(other.beginIt), endIt(other.endIt) {}

    IteratorPair(TIterator beginIt, TIterator endIt) : beginIt(beginIt), endIt(endIt) {}

    TIterator begin() { return beginIt; }
    TIterator end() { return endIt; }

private:
    TIterator beginIt;
    TIterator endIt;
};

// Sequence containers

template <typename T>
class Array
{
public:
// Constructors
    Array();
    Array(const Array & other);
    Array(Array && other);
    
    Array(int count, T element = T());
    Array(int count, const T * elementPtr);

    ~Array();
    
// Attributes
    T &         At(int index);
    const T &   At(int index) const;

    bool        Contains(T element) const;

    bool        Empty() const;
    int         Count() const;

    // Iterator.
    using ElementIt = IteratorPair<T *>;
    using IndexIt = IteratorPair<IntIterator>;
    using ReversedElementIt = IteratorPair<MirroredPtr<T>>;
    using ReversedIndexIt = IteratorPair<MirroredIntIterator>;
    ElementIt           Elements() const;
    IndexIt             Indexes() const;
    ReversedElementIt   ElementsReversed() const;
    ReversedIndexIt     IndexesReversed() const;

// Operations
    void        Add(T element);
    void        Append(const Array & other);

    void        Insert(int index, T element, int count = 1);
    void        Remove(int index, int count = 1);

    void        RemoveAll();

    T &         operator[] (int index);
    const T &   operator[] (int index) const;

// Overridables

// Implementation
private:
    static T * Alloc(int count)
    {
        ASSERT(count == CeilPowOf2(count));
        return (T *)LowLevel::Alloc(count * sizeof(T));
    }
    static void Free(T * data)
    {
        LowLevel::Free(data);
    }
    static void CopyConstruct(T * to, T value, int count)
    {
        for (; count > 0; ++to, --count)
        {
            new (to) T(value);
        }
    }
    static void CopyConstruct(T * to, const T * from, int count)
    {
        for (; count > 0; ++to, ++from, --count)
        {
            new (to) T(*from);
        }
    }
    static void MoveConstruct(T * to, T * from, int count)
    {
        for (; count > 0; ++to, ++from, --count)
        {
            new (to) T(std::move(*from));
        }
    }
    static void Destruct(T * to, int count = 1)
    {
        for (; count > 0; ++to, --count)
        {
            to->~T();
        }
    }
    // move shift, copy shift, value shift
    static void ShiftRight(T * begin, T * end, int count)
    {
        ASSERT(count > 0);
        // [end, end + count) is invalid (uninitialized, or moved away, or destructed)
        T * newEnd = end + count;
        while (begin != end)
        {
            --newEnd, --end;
            new (newEnd) T(std::move(*end));
        }
        // [begin, begin + count) is invalid
    }
    static void ShiftLeft(T * begin, T * end, int count)
    {
        ASSERT(count > 0);
        // [begin - count, begin) is invalid
        T * newBegin = begin - count;
        while (begin != end)
        {
            new (newBegin) T(std::move(*begin));
            ++newBegin, ++begin;
        }
        // [end - count, end) is invalid
    }
    void Expand(int newMaxCount)
    {
        newMaxCount = CeilPowOf2(newMaxCount);
        if (elemMaxCount != newMaxCount)
        {
            T * newData = Alloc(newMaxCount);
            if (0 < elemCount)
            {
                MoveConstruct(newData, elemData, elemCount);
                Free(elemData);
            }
            elemData = newData;
            elemMaxCount = newMaxCount;
        }
        ASSERT(elemData);
    }

    int elemMaxCount;
    int elemCount;
    T * elemData;
};

template <typename T>
Array<T>::Array()
    : elemMaxCount(0)
    , elemCount(0)
    , elemData(nullptr)
{
}

template <typename T>
Array<T>::Array(const Array & other)
    : elemMaxCount(other.elemMaxCount)
    , elemCount(other.elemCount)
    , elemData(nullptr)
{
    if (elemMaxCount > 0)
    {
        elemData = Alloc(elemMaxCount);
        CopyConstruct(elemData, other.elemData, elemCount);
    }
}

template <typename T>
Array<T>::Array(Array && other)
    : elemMaxCount(other.elemMaxCount)
    , elemCount(other.elemCount)
    , elemData(other.elemData)
{
    other.elemMaxCount = 0;
    other.elemCount = 0;
    other.elemData = nullptr;
}

template <typename T>
Array<T>::Array(int count, T element)
    : elemMaxCount(CeilPowOf2(count))
    , elemCount(count)
    , elemData(nullptr)
{
    if (elemMaxCount > 0)
    {
        elemData = Alloc(elemMaxCount);
        CopyConstruct(elemData, element, elemCount);
    }
}

template <typename T>
Array<T>::Array(int count, const T * elementPtr)
    : elemMaxCount(CeilPowOf2(count))
    , elemCount(count)
    , elemData(nullptr)
{
    if (elemMaxCount > 0)
    {
        elemData = Alloc(elemMaxCount);
        CopyConstruct(elemData, elementPtr, elemCount);
    }
}

template <typename T>
Array<T>::~Array()
{
    if (elemData)
    {
        Free(elemData);
    }
}

template <typename T>
T & Array<T>::At(int index)
{
    ASSERT(0 <= index && index < elemCount);
    return elemData[index];
}

template <typename T>
const T & Array<T>::At(int index) const
{
    ASSERT(0 <= index && index < elemCount);
    return elemData[index];
}

template <typename T>
bool Array<T>::Contains(T element) const
{
    for (T * elemIt = elemData;
         elemIt != elemData + elemCount;
         ++elemIt)
    {
        if (*elemIt == element)
            return true;
    }
    return false;
}

template <typename T>
bool Array<T>::Empty() const
{
    return elemCount == 0;
}

template <typename T>
int Array<T>::Count() const
{
    return elemCount;
}

template <typename T>
typename Array<T>::ElementIt Array<T>::Elements() const
{
    return Empty() ? ElementIt() : ElementIt(elemData, elemData + elemCount);
}

template <typename T>
typename Array<T>::IndexIt Array<T>::Indexes() const
{
    return Empty() ? IndexIt() : IndexIt(0, elemCount);
}

template <typename T>
typename Array<T>::ReversedElementIt Array<T>::ElementsReversed() const
{
    return Empty() ? ReversedElementIt() : ReversedElementIt(elemData + elemCount, elemData);
}

template <typename T>
typename Array<T>::ReversedIndexIt Array<T>::IndexesReversed() const
{
    return Empty() ? ReversedIndexIt() : ReversedIndexIt(elemCount, 0);
}

template <typename T>
void Array<T>::Add(T element)
{
    Expand(elemCount + 1);

    CopyConstruct(elemData + elemCount, element, 1);
    ++elemCount;
}

template <typename T>
void Array<T>::Append(const Array & other)
{
    Expand(elemCount + other.elemCount);

    CopyConstruct(elemData + elemCount, other.elemData, other.elemCount);
    elemCount += other.elemCount;
}

template <typename T>
void Array<T>::Insert(int index, T element, int count)
{
    ASSERT(0 <= index && index <= elemCount);
    ASSERT(0 < count);
    Expand(elemCount + count);

    ShiftRight(elemData + index, elemData + elemCount, count);
    CopyConstruct(elemData + index, element, count);
    elemCount += count;
}

template <typename T>
void Array<T>::Remove(int index, int count)
{
    ASSERT(0 <= index && index < elemCount);
    ASSERT(0 < count && count <= (elemCount - index));

    ShiftLeft(elemData + index + count, elemData + elemCount, count);
    elemCount -= count;
}

template <typename T>
void Array<T>::RemoveAll()
{
    if (0 < elemCount)
    {
        Destruct(elemData, elemCount);
        elemCount = 0;
    }
}

template <typename T>
T & Array<T>::operator[] (int index)
{
    ASSERT(0 <= index && index < elemCount);
    return elemData[index];
}

template <typename T>
const T & Array<T>::operator[] (int index) const
{
    ASSERT(0 <= index && index < elemCount);
    return elemData[index];
}

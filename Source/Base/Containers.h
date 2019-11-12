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
    Array()
        : nCap(0)
        , nCount(0)
        , pData(nullptr)
    {
    }
    Array(const Array & other)
        : nCap(other.nCap)
        , nCount(other.nCount)
        , pData(nullptr)
    {
        if (nCap > 0)
        {
            pData = _Alloc(nCap);
            _CopyConstruct(pData, other.pData, nCount);
        }
    }
    Array(Array && other)
        : nCap(other.nCap)
        , nCount(other.nCount)
        , pData(other.pData)
    {
        other.nCap = 0;
        other.nCount = 0;
        other.pData = nullptr;
    }
    
    Array(int count, T element = T())
        : nCap(CeilPowOf2(count))
        , nCount(count)
        , pData(nullptr)
    {
        if (nCap > 0)
        {
            pData = _Alloc(nCap);
            _CopyConstruct(pData, element, nCount);
        }
    }
    Array(int count, const T * elementPtr)
        : nCap(CeilPowOf2(count))
        , nCount(count)
        , pData(nullptr)
    {
        if (nCap > 0)
        {
            pData = _Alloc(nCap);
            _CopyConstruct(pData, elementPtr, nCount);
        }
    }

    ~Array()
    {
        if (pData)
        {
            _Free(pData);
        }
    }
    
// Attributes
    const T &   At(int index) const
    {
        ASSERT(0 <= index && index < nCount);
        return pData[index];
    }
    T &         At(int index)
    {
        ASSERT(0 <= index && index < nCount);
        return pData[index];
    }

    bool        IsEmpty() const
    {
        return nCount == 0;
    }

    int         Count() const
    {
        return nCount;
    }

// Operations
    bool        Contains(T element) const
    {
        for (T * elemIt = pData;
             elemIt != pData + nCount;
             ++elemIt)
        {
            if (*elemIt == element)
                return true;
        }
        return false;
    }


    void        Add(T element)
    {
        Expand(nCount + 1);

        _CopyConstruct(pData + nCount, element, 1);
        ++nCount;
    }
    void        Append(const Array & other)
    {
        Expand(nCount + other.nCount);

        _CopyConstruct(pData + nCount, other.pData, other.nCount);
        nCount += other.nCount;
    }

    void        InsertAt(int index, T element, int count = 1)
    {
        ASSERT(0 <= index && index <= nCount);
        ASSERT(0 < count);
        Expand(nCount + count);

        _ShiftRight(pData + index, pData + nCount, count);
        _CopyConstruct(pData + index, element, count);
        nCount += count;
    }
    void        RemoveAt(int index, int count = 1)
    {
        ASSERT(0 <= index && index < nCount);
        ASSERT(0 < count && count <= (nCount - index));

        _ShiftLeft(pData + index + count, pData + nCount, count);
        nCount -= count;
    }

    void        RemoveAll()
    {
        if (0 < nCount)
        {
            _Destruct(pData, nCount);
            nCount = 0;
        }
    }

    T &         operator[] (int index)
    {
        ASSERT(0 <= index && index < nCount);
        return pData[index];
    }
    const T &   operator[] (int index) const
    {
        ASSERT(0 <= index && index < nCount);
        return pData[index];
    }

    // Iterator.
    using ElementIt = IteratorPair<T *>;
    using IndexIt = IteratorPair<IntIterator>;
    using ReversedElementIt = IteratorPair<MirroredPtr<T>>;
    using ReversedIndexIt = IteratorPair<MirroredIntIterator>;
    ElementIt           Elements() const
    {
        return IsEmpty() ? ElementIt() : ElementIt(pData, pData + nCount);
    }
    IndexIt             Indexes() const
    {
        return IsEmpty() ? IndexIt() : IndexIt(0, nCount);
    }
    ReversedElementIt   ElementsReversed() const
    {
        return IsEmpty() ? ReversedElementIt() : ReversedElementIt(pData + nCount, pData);
    }
    ReversedIndexIt     IndexesReversed() const
    {
        return IsEmpty() ? ReversedIndexIt() : ReversedIndexIt(nCount, 0);
    }

// Overridables

// Implementation
private:
    int nCap;
    int nCount;
    T * pData;

    static T * _Alloc(int count)
    {
        ASSERT(count == CeilPowOf2(count));
        return (T *)LowLevel::Alloc(count * sizeof(T));
    }
    static void _Free(T * data)
    {
        LowLevel::Free(data);
    }
    static void _CopyConstruct(T * to, T value, int count)
    {
        for (; count > 0; ++to, --count)
        {
            new (to) T(value);
        }
    }
    static void _CopyConstruct(T * to, const T * from, int count)
    {
        for (; count > 0; ++to, ++from, --count)
        {
            new (to) T(*from);
        }
    }
    static void _MoveConstruct(T * to, T * from, int count)
    {
        for (; count > 0; ++to, ++from, --count)
        {
            new (to) T(std::move(*from));
        }
    }
    static void _Destruct(T * to, int count = 1)
    {
        for (; count > 0; ++to, --count)
        {
            to->~T();
        }
    }
    // move shift, copy shift, value shift
    static void _ShiftRight(T * begin, T * end, int count)
    {
        ASSERT(count > 0);
        // [end, end + count) is invalid (uninitialized, or moved away, or destructed)
        T * nNewEnd = end + count;
        while (begin != end)
        {
            --nNewEnd, --end;
            new (nNewEnd) T(std::move(*end));
        }
        // [begin, begin + count) is invalid
    }
    static void _ShiftLeft(T * begin, T * end, int count)
    {
        ASSERT(count > 0);
        // [begin - count, begin) is invalid
        T * nNewBegin = begin - count;
        while (begin != end)
        {
            new (nNewBegin) T(std::move(*begin));
            ++nNewBegin, ++begin;
        }
        // [end - count, end) is invalid
    }
    void Expand(int nNewMaxCount)
    {
        nNewMaxCount = CeilPowOf2(nNewMaxCount);
        if (nCap != nNewMaxCount)
        {
            T * pNewData = _Alloc(nNewMaxCount);
            if (0 < nCount)
            {
                _MoveConstruct(pNewData, pData, nCount);
                _Free(pData);
            }
            pData = pNewData;
            nCap = nNewMaxCount;
        }
        ASSERT(pData);
    }
};

template <typename T>
class List
{
public:
    using Position = void *;

// Constructors
    List()
        : nCount(0)
        , pHead(nullptr)
        , pTail(nullptr)
    {
    }

    List(const List & other)
        : nCount(other.nCount)
        , pHead(other.pHead)
        , pTail(other.pTail)
    {
        other.nCount = 0;
        other.pHead = other.pTail = nullptr;
    }
    List(List && other)
        : nCount(0)
        , pHead(nullptr)
        , pTail(nullptr)
    {
        Node * pNode;
        for (pNode = other.pHead;
             pNode;
             pNode = pNode->pNext)
        {
            AddTail(pNode->value);
        }
    }

    ~List()
    {
        RemoveAll();
    }
	
// Attributes
    int Count()
    {
        return nCount;
    }
    bool Empty()
    {
        return nCount == 0;
    }

    T & Head()
    {
        ASSERT(!Empty());
        return pHead->value;
    }

    T & Tail()
    {
        ASSERT(!Empty());
        return pTail->value;
    }
    T & At(Position pos)
    {
        ASSERT(pos);
        return static_cast<Node *>(pos)->value;
    }

    Position HeadPos()
    {
        ASSERT(!Empty());
        return pHead;
    }
    Position TailPos()
    {
        ASSERT(!Empty());
        return pTail;
    }

// Operations
    void AddHead(T element)
    {
        Node * pNew = _Alloc();
        _Construct(pNew, element);

        if (Empty())
        {
            pHead = pNew;
            pTail = pNew;
            nCount = 1;
        }
        else
        {
            _Link(pNew, pHead);
            pHead = pNew;
            ++nCount;
        }
    }
    void AddTail(T element)
    {
        Node * pNew = _Alloc();
        _Construct(pNew, element);

        if (Empty())
        {
            pHead = pNew;
            pTail = pNew;
            nCount = 1;
        }
        else
        {
            _Link(pTail, pNew);
            pTail = pNew;
            ++nCount;
        }
    }

    Position FindElementPos(T element)
    {
        if (Empty())
            return nullptr;

        Node * pNode;
        for (pNode = pHead;
             pNode != pTail;
             pNode = pNode->pNext)
        {
            if (pNode->value == element)
                return pNode;
        }
        return nullptr;
    }
    Position FindIndexPos(int index)
    {
        ASSERT(0 <= index && index < Count());

        if (Empty())
            return nullptr;

        Node * pNode;
        for (pNode = pHead;
             pNode != pTail && 0 < index;
             pNode = pNode->pNext, --index)
        {}
        return pNode;
    }
    Position FindNextPos(Position pos)
    {
        ASSERT(pos);
        Node * pNode = static_cast<Node *>(pos);
        return pNode->pNext;
    }
    Position FindPrevPos(Position pos)
    {
        ASSERT(pos);
        Node * pNode = static_cast<Node *>(pos);
        return pNode->pPrev;
    }

    void InsertAfter(Position pos, T element)
    {
        ASSERT(pos);
        Node * pLeft = static_cast<Node *>(pos);

        Node * pNew = _Alloc();
        _Construct(pNew, element);

        if (pLeft == pTail)
        {
            _Link(pTail, pNew);
            pTail = pNew;
            ++nCount;
        }
        else
        {
            Node * pRight = pLeft->pNext;
            _Unlink(pLeft, pRight);
            _Link(pLeft, pNew);
            _Link(pNew, pRight);
            ++nCount;
        }
    }
    void InsertBefore(Position pos, T element)
    {
        ASSERT(pos);
        Node * pRight = static_cast<Node *>(pos);

        Node * pNew = _Alloc();
        _Construct(pNew, element);

        if (pRight == pHead)
        {
            _Link(pNew, pHead);
            pHead = pNew;
            ++nCount;
        }
        else
        {
            Node * pLeft = pRight->pPrev;
            _Unlink(pLeft, pRight);
            _Link(pLeft, pNew);
            _Link(pNew, pRight);
            ++nCount;
        }
    }
    void RemoveHead()
    {
        ASSERT(!Empty());
        Node * pDel = pHead;

        if (Count() == 1)
        {
            pHead = pTail = nullptr;
            nCount = 0;
        }
        else
        {
            pHead = pHead->pNext;
            _Unlink(pDel, pHead);
            --nCount;
        }

        _Destruct(pDel);
        _Free(pDel);
    }
    void RemoveTail()
    {
        ASSERT(!Empty());
        Node * pDel = pTail;

        if (Count() == 1)
        {
            pHead = pTail = nullptr;
            nCount = 0;
        }
        else
        {
            pTail = pTail->pPrev;
            _Unlink(pTail, pDel);
            --nCount;
        }

        _Destruct(pDel);
        _Free(pDel);
    }
    void RemoveAt(Position pos)
    {
        ASSERT(pos);
        Node * pDel = static_cast<Node *>(pos);

        Node * pLeft = pDel->pPrev;
        Node * pRight = pDel->pNext;

        if (pLeft)
            _Unlink(pLeft, pDel);
        else
            pHead = pRight;

        if (pRight)
            _Unlink(pDel, pRight);
        else
            pTail = pLeft;

        if (pLeft && pRight)
            _Link(pLeft, pRight);

        _Destruct(pDel);
        _Free(pDel);
        --nCount;
    }
    void RemoveAll()
    {
        Node * pDel;
        Node * pIt = pHead;
        while (pIt)
        {
            pDel = pIt;
            pIt = pIt->pNext;
            _Destruct(pDel);
            _Free(pDel);
        }
        pHead = pTail = nullptr;
        nCount = 0;
    }


// Implementation
private:
    struct Node
    {
        Node * pPrev;
        Node * pNext;
        T value;
    };

    int nCount;
    Node * pHead;
    Node * pTail;

    static inline Node* _Alloc()
    {
        Node * pNode = (Node *)LowLevel::Alloc(sizeof(Node));
        pNode->pPrev = nullptr;
        pNode->pNext = nullptr;
        return pNode;
    }
    static inline void  _Free(Node * pNode)
    {
        ASSERT(pNode);
        LowLevel::Free(pNode);
    }
    static inline void  _Link(Node * pLeft, Node * pRight)
    {
        ASSERT(pLeft && pRight);
        ASSERT(!pLeft->pNext && !pRight->pPrev);
        pLeft->pNext = pRight;
        pRight->pPrev = pLeft;
    }
    static inline void  _Unlink(Node * pLeft, Node * pRight)
    {
        ASSERT(pLeft && pRight);
        ASSERT(pLeft->pNext == pRight && pRight->pPrev == pLeft);
        pLeft->pNext = nullptr;
        pRight->pPrev = nullptr;
    }
    static inline void  _Construct(Node * pNode, T element)
    {
        ASSERT(pNode);
        new (&pNode->value) T(element);
    }
    static inline void  _Destruct(Node * pNode)
    {
        ASSERT(pNode);
        pNode->value.~T();
    }
};

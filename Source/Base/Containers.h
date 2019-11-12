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
    const T &   At(int index) const;
    T &         At(int index);

    bool        IsEmpty() const;
    int         Count() const;

// Operations
    bool        Contains(T element) const;

    void        Add(T element);
    void        Append(const Array & other);

    void        InsertAt(int index, T element, int count = 1);
    void        RemoveAt(int index, int count = 1);
    void        RemoveAll();

    T &         operator[] (int index);
    const T &   operator[] (int index) const;

    // Iterator.
    using ElementIt = IteratorPair<T *>;
    using IndexIt = IteratorPair<IntIterator>;
    using ReversedElementIt = IteratorPair<MirroredPtr<T>>;
    using ReversedIndexIt = IteratorPair<MirroredIntIterator>;
    ElementIt           Elements() const;
    IndexIt             Indexes() const;
    ReversedElementIt   ElementsReversed() const;
    ReversedIndexIt     IndexesReversed() const;

// Overridables

// Implementation
private:
    int nCap;
    int nCount;
    T * pData;

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
        T * nNewEnd = end + count;
        while (begin != end)
        {
            --nNewEnd, --end;
            new (nNewEnd) T(std::move(*end));
        }
        // [begin, begin + count) is invalid
    }
    static void ShiftLeft(T * begin, T * end, int count)
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
            T * pNewData = Alloc(nNewMaxCount);
            if (0 < nCount)
            {
                MoveConstruct(pNewData, pData, nCount);
                Free(pData);
            }
            pData = pNewData;
            nCap = nNewMaxCount;
        }
        ASSERT(pData);
    }
};

template <typename T>
Array<T>::Array()
    : nCap(0)
    , nCount(0)
    , pData(nullptr)
{
}

template <typename T>
Array<T>::Array(const Array & other)
    : nCap(other.nCap)
    , nCount(other.nCount)
    , pData(nullptr)
{
    if (nCap > 0)
    {
        pData = Alloc(nCap);
        CopyConstruct(pData, other.pData, nCount);
    }
}

template <typename T>
Array<T>::Array(Array && other)
    : nCap(other.nCap)
    , nCount(other.nCount)
    , pData(other.pData)
{
    other.nCap = 0;
    other.nCount = 0;
    other.pData = nullptr;
}

template <typename T>
Array<T>::Array(int count, T element)
    : nCap(CeilPowOf2(count))
    , nCount(count)
    , pData(nullptr)
{
    if (nCap > 0)
    {
        pData = Alloc(nCap);
        CopyConstruct(pData, element, nCount);
    }
}

template <typename T>
Array<T>::Array(int count, const T * elementPtr)
    : nCap(CeilPowOf2(count))
    , nCount(count)
    , pData(nullptr)
{
    if (nCap > 0)
    {
        pData = Alloc(nCap);
        CopyConstruct(pData, elementPtr, nCount);
    }
}

template <typename T>
Array<T>::~Array()
{
    if (pData)
    {
        Free(pData);
    }
}

template <typename T>
T & Array<T>::At(int index)
{
    ASSERT(0 <= index && index < nCount);
    return pData[index];
}

template <typename T>
const T & Array<T>::At(int index) const
{
    ASSERT(0 <= index && index < nCount);
    return pData[index];
}

template <typename T>
bool Array<T>::Contains(T element) const
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

template <typename T>
bool Array<T>::IsEmpty() const
{
    return nCount == 0;
}

template <typename T>
int Array<T>::Count() const
{
    return nCount;
}

template <typename T>
typename Array<T>::ElementIt Array<T>::Elements() const
{
    return IsEmpty() ? ElementIt() : ElementIt(pData, pData + nCount);
}

template <typename T>
typename Array<T>::IndexIt Array<T>::Indexes() const
{
    return IsEmpty() ? IndexIt() : IndexIt(0, nCount);
}

template <typename T>
typename Array<T>::ReversedElementIt Array<T>::ElementsReversed() const
{
    return IsEmpty() ? ReversedElementIt() : ReversedElementIt(pData + nCount, pData);
}

template <typename T>
typename Array<T>::ReversedIndexIt Array<T>::IndexesReversed() const
{
    return IsEmpty() ? ReversedIndexIt() : ReversedIndexIt(nCount, 0);
}

template <typename T>
void Array<T>::Add(T element)
{
    Expand(nCount + 1);

    CopyConstruct(pData + nCount, element, 1);
    ++nCount;
}

template <typename T>
void Array<T>::Append(const Array & other)
{
    Expand(nCount + other.nCount);

    CopyConstruct(pData + nCount, other.pData, other.nCount);
    nCount += other.nCount;
}

template <typename T>
void Array<T>::InsertAt(int index, T element, int count)
{
    ASSERT(0 <= index && index <= nCount);
    ASSERT(0 < count);
    Expand(nCount + count);

    ShiftRight(pData + index, pData + nCount, count);
    CopyConstruct(pData + index, element, count);
    nCount += count;
}

template <typename T>
void Array<T>::RemoveAt(int index, int count)
{
    ASSERT(0 <= index && index < nCount);
    ASSERT(0 < count && count <= (nCount - index));

    ShiftLeft(pData + index + count, pData + nCount, count);
    nCount -= count;
}

template <typename T>
void Array<T>::RemoveAll()
{
    if (0 < nCount)
    {
        Destruct(pData, nCount);
        nCount = 0;
    }
}

template <typename T>
T & Array<T>::operator[] (int index)
{
    ASSERT(0 <= index && index < nCount);
    return pData[index];
}

template <typename T>
const T & Array<T>::operator[] (int index) const
{
    ASSERT(0 <= index && index < nCount);
    return pData[index];
}

// Index Type: int
// Element Arg Type: T
// Element Ref Type: T &
template <typename T>
class List
{
public:
    using Position = void *;

// Constructors
	List();
	List(const List & other);
	List(List && other);
	
// Attributes
    int Count();
    bool Empty();

    T & Head();
    T & Tail();
    T & At(Position pos);

    Position HeadPos();
    Position TailPos();

// Operations
    void AddHead(T element);
    void AddTail(T element); // Add

    Position FindElementPos(T element);
    Position FindIndexPos(int index);
    Position FindNextPos(Position pos);
    Position FindPrevPos(Position pos);

    void InsertAfter(Position pos, T element);
    void InsertBefore(Position pos, T element);
    void RemoveHead();
    void RemoveTail();
    void RemoveAt(Position pos);
    void RemoveAll();

// Implementation
private:
    struct Node
    {
        Node * pPrev;
        Node * pNext;
        T value;
    };

    int nCount;
    Node origin; // T supports default construct

    //static inline void  _Init(List & list);
    static inline Node* _Alloc()
    {
        Node * pNode = LowLevel::Alloc(sizeof(Node));
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
        new (pNode->value) T(element);
    }
    static inline void  _Destruct(Node * pNode)
    {
        ASSERT(pNode);
        pNode->value->~T();
    }
    static inline void  _InsertEmpty(List & list, Node * pNew)
    {
        ASSERT(list.Empty() && pNew);
        list.pHead = pNew;
        list.pTail = pNew;
        list.nCount = 1;
    }
    static inline void  _InsertHead(List & list, Node * pNew)
    {
        ASSERT(list.Empty() && pNew);
        _Link(list.pHead, pNew);
        list.pHead = pNew;
        ++list.nCount;
    }
    static inline void  _InsertTail(List & list, Node * pNew)
    {
        ASSERT(list.Empty() && pNew);
        _Link(pNew, list.pTail);
        list.pTail = pNew;
        ++list.nCount;
    }
    static inline void  _InsertBefore(List & list, Node * pPos, Node * pNew)
    {
        ASSERT(pPos && pNew);
        
    }
    static inline void  _InsertAfter(List & list, Node * pPos, Node * pNew)
    {
        ASSERT(pPos && pNew);
    }
};

template <typename T>
List<T>::List()
    : nCount(0)
    , pHead(nullptr)
    , pTail(nullptr)
{
}

template <typename T>
List<T>::List(List && other)
    : nCount(other.nCount)
    , pHead(other.pHead)
    , pTail(other.pTail)
{
    other.nCount = 0;
    other.pHead = other.pTail = nullptr;
}

template <typename T>
List<T>::List(const List & other)
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

template <typename T>
int List<T>::Count()
{
    return nCount;
}

template <typename T>
bool List<T>::Empty()
{
    return nCount == 0;
}

template <typename T>
T & List<T>::Head()
{
    ASSERT(!Empty());
    return pHead->value;
}

template <typename T>
T & List<T>::Tail()
{
    ASSERT(!Empty());
    return pTail->value;
}

template <typename T>
T & List<T>::At(Position pos)
{
    ASSERT(pos);
    return static_cast<Node *>(pos)->value;
}

template <typename T>
typename List<T>::Position List<T>::HeadPos()
{
    ASSERT(!Empty());
    return pHead;
}

template <typename T>
typename List<T>::Position List<T>::TailPos()
{
    ASSERT(!Empty());
    return pTail;
}

template <typename T>
void List<T>::AddHead(T element)
{
    Node * pNode = _Alloc();
    _Construct(pNode, element);

    if (Empty())
        _InsertEmpty(*this, pNode);
    else
        _InsertBefore(*this, pHead, pNode);
}

template <typename T>
void List<T>::AddTail(T element)
{
    Node * pNode = _Alloc();
    _Construct(pNode, element);

    if (Empty())
        _InsertEmpty(*this, pNode);
    else
        _InsertAfter(*this, pTail, pNode);
}

template <typename T>
typename List<T>::Position List<T>::FindElementPos(T element)
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

template <typename T>
typename List<T>::Position List<T>::FindIndexPos(int index)
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

template <typename T>
typename List<T>::Position List<T>::FindNextPos(List<T>::Position pos)
{
    ASSERT(pos);
    Node * pNode = static_cast<Node *>(pos);

    return pNode->pNext;
}

template <typename T>
typename List<T>::Position List<T>::FindPrevPos(List<T>::Position pos)
{
    ASSERT(pos);
    Node * pNode = static_cast<Node *>(pos);

    return pNode->pPrev;
}

template <typename T>
void List<T>::InsertAfter(Position pos, T element)
{
    ASSERT(pos);
    Node * pNode = static_cast<Node *>(pos);

    Node * pNew = _Alloc();
    _Construct(pNew, element);

    if (pNode == pTail)
        _InsertTail(*this, pNew);
    else
        _InsertAfter(*this, pNode, pNew);
}

template <typename T>
void List<T>::InsertBefore(Position pos, T element)
{
    ASSERT(pos);
    Node * pNode = static_cast<Node *>(pos);

    Node * pNew = _Alloc();
    _Construct(pNew, element);

    if (pNode == pHead)
        _InsertHead(*this, pNew);
    else
        _InsertBefore(*this, pNode, pNew);
}

template <typename T>
void List<T>::RemoveHead()
{
    ASSERT(!Empty());

    if (Count() == 1)
    {
        _Destruct(pHead);
        _Free(pHead);
        pHead = pTail = nullptr;
        nCount = 0;
    }
    else
    {
        Node * pNode = pHead;
        pHead = pHead->pNext;
        _Unlink(pNode, pHead);
        _Destruct(pNode);
        _Free(pNode);
        --nCount;
    }
}

template <typename T>
void List<T>::RemoveTail()
{
    ASSERT(!Empty());

    if (Count() == 1)
    {
        _Destruct(pHead);
        _Free(pHead);
        pHead = pTail = nullptr;
        nCount = 0;
    }
    else
    {
        Node * pNode = pTail;
        pTail = pTail->pPrev;
        _Unlink(pTail, pNode);
        _Destruct(pNode);
        _Free(pNode);
        --nCount;
    }
}

template <typename T>
void List<T>::RemoveAt(Position pos)
{
    ASSERT(pos);
    Node * pNode = static_cast<Node *>(pos);

    Node * pLeft = pNode->pPrev;
    Node * pRight = pNode->pNext;
    
    if (pLeft)
        _Unlink(pLeft, pNode);
    else
        pHead = pRight;

    if (pRight)
        _Unlink(pNode, pRight);
    else
        pTail = pLeft;

    if (pLeft && pRight)
        _Link(pLeft, pRight);

    _Destruct(pNode);
    _Free(pNode);
    --nCount;
}

template <typename T>
void List<T>::RemoveAll()
{
    Node * pCur;
    Node * pNxt = pHead;
    while (pNxt)
    {
        pCur = pNxt;
        pNxt = pNxt->pNext;
        _Destruct(pCur);
        _Free(pCur);
    }
    pHead = pTail = nullptr;
    nCount = 0;
}

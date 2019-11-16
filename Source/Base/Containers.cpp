#include "Containers.h"

namespace containers {

template <>
UINT HashKey(int key)
{
    return UINT(key) * 2654435761;
}

}
#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

#include <map>

using namespace containers;

bool EQ(Array<int> & actual, std::vector<int> expect)
{
    if (actual.Count() != (int)expect.size())
        return false;
    for (int i = 0; i < actual.Count(); ++i)
    {
        if (actual[i] != expect[i])
            return false;
    }
    return true;
}
bool EQ(List<int> & actual, std::vector<int> expect)
{
    if (actual.Count() != (int)expect.size())
        return false;
    auto it = actual.Empty() ? nullptr : actual.HeadPos();
    for (int i = 0; i < actual.Count(); ++i)
    {
        if (actual.At(it) != expect[i])
            return false;
        it = actual.FindNextPos(it);
    }
    return true;
}
bool EQ(Map<int, int> & actual, std::map<int, int> expect)
{
    if (actual.Count() != (int)expect.size())
        return false;
    if (!actual.Empty())
    {
        int key, value;
        auto it = actual.GetStartPos();
        while (actual.GetNextAssoc(it, key, value))
        {
            if (expect.find(key) == expect.end() ||
                expect.at(key) != value)
                return false;
        }
    }
    return true;
}

#undef EXPECT
#define EXPECT(actual) EXPECT_EQ(actual, true)

TEST(Array_API)
{
    // Create an empty array.
    Array<int> a0;

    // Create an array of 20 1's.
    Array<int> a1(5, 1);
    
    EXPECT(EQ(  a1,     {1,1,1,1,1}  ));

    // Create an arrary from C-array.
    int cstyle[] = {1, 2, 3, 4, 5};
    Array<int> a2(5, cstyle);
    
    EXPECT(EQ(  a2,     {1,2,3,4,5}  ));

    // Insert elements at tail.
    Array<int> a3;
    a3.Add(0);
    a3.Append(a2);
    a3.Append(a3); // append self !

    EXPECT(EQ(  a3,     {0,1,2,3,4,5,0,1,2,3,4,5}  ));

    // Insert elements at index.
    Array<int> a4;
    a4.InsertAt(0, 2); // 2
    a4.InsertAt(1, 4); // 2,4
    a4.InsertAt(0, 1); // 1,2,4
    a4.InsertAt(2, 3); // 1,2,3,4

    EXPECT(EQ(  a4,     {1,2,3,4}  ));

    // Remove elements at index.
    a4.RemoveAt(0); // 2,3,4
    a4.RemoveAt(1); // 2,4
    a4.RemoveAt(1); // 2
    a4.RemoveAll();

    EXPECT(EQ(  a4,     {}  ));

    // Query elements.
    Array<int> a5(5, cstyle);
    a5.At(0); // 1
    a5.At(4) += 1; // 6
    a5.Contains(4); // true
    a5.Contains(5); // false
    a5.IsEmpty(); // false

    EXPECT(EQ(  a5,     {1,2,3,4,6}  ));

    // Enumerate elements.
    Array<int> a6(5, cstyle);
    for (int element : a6.Elements())
        element;
    for (int element : a6.ElementsReversed())
        element;
    for (int index : a6.Indexes())
        a6.At(index);
    for (int index : a6.IndexesReversed())
        a6.At(index);
    /*
        for (int [index, element] : a6.IndexElementPairs())
        for (int [index, element] : a6.IndexElementPairsReserved())
    */
}

TEST(List_API)
{
    // Create an empty list.
    List<int> l0;

    // Insert elements at boundary.
    List<int> l1;
    l1.AddHead(2); // 2
    l1.AddTail(4); // 2,4

    EXPECT(EQ(  l1,     {2,4}  ));

    // Insert elements at position.
    l1.InsertAfter(l1.FindElementPos(2), 3); // 2,3,4
    l1.InsertBefore(l1.FindIndexPos(0), 1); // 1,2,3,4

    EXPECT(EQ(  l1,     {1,2,3,4}  ));

    // Remove elements at position.
    l1.RemoveAt(l1.FindIndexPos(1)); // 1,3,4

    EXPECT(EQ(  l1,     {1,3,4}  ));

    // Remove elements at boundary.
    l1.RemoveHead(); // 3,4
    l1.RemoveTail(); // 4
    l1.RemoveAll();

    EXPECT(EQ(  l1,     {}  ));

    // Query elements.
    List<int> l2;
    l2.AddTail(1);
    l2.AddTail(2);
    l2.AddTail(3);
    l2.Count(); // 3
    l2.Empty(); // false
    l2.Head(); // 1
    l2.At(l2.FindIndexPos(1)); // 2
    l2.Tail(); // 3

    EXPECT(EQ(  l2,     {1,2,3}  ));
    
    // Enumerate elements.
    List<int> l3;
    l3.AddTail(1);
    l3.AddTail(2);
    l3.AddTail(3);

    EXPECT(EQ(  l3,     {1,2,3}  ));

    for (auto p = l3.HeadPos(); p; p = l3.FindNextPos(p))
    {
        l3.At(p);
    }
}

TEST(Map_API)
{
    // Create an empty map.
    Map<int, int> m0;

    EXPECT(EQ(  m0,     {} ));

    // Insert elements.
    m0.Insert(1, 2);
    m0.Insert(2, 3);
    m0.Insert(3, 5);
    m0.Insert(4, 7);

    EXPECT(EQ(  m0,     {{1,2},{2,3},{3,5},{4,7}} ));

    // Query elements.
    int value;
    m0.At(3); // 5
    m0.Lookup(2, value); // true, 3
    m0.Lookup(10, value); // false

    // Remove elements.
    m0.Remove(1);
    m0.Remove(4);

    EXPECT(EQ(  m0,     {{2,3},{3,5}} ));

    // Enumerate elements.
    int key, val;
    for (auto it = m0.GetStartPos();
         m0.GetNextAssoc(it, key, val);
         )
    {
        key, val;
    }
}

#endif

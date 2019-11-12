#include "Containers.h"

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

TEST(Array_API)
{
    // Create an empty array.
    Array<int> a0;

    // Create an array of 100 1's.
    Array<int> a1(20, 1);

    // Create an arrary from C-array.
    int cstyle[] = {1, 2, 3, 4, 5};
    Array<int> a2(5, cstyle);

    // Insert elements at tail.
    Array<int> a3;
    a3.Add(0);
    a3.Append(a2);
    a3.Append(a3); // append self !

    // Insert elements at index.
    Array<int> a4;
    a4.InsertAt(0, 2); // 2
    a4.InsertAt(1, 4); // 2,4
    a4.InsertAt(0, 1); // 1,2,4
    a4.InsertAt(2, 3); // 1,2,3,4

    // Remove elements at index.
    a4.RemoveAt(0); // 2,3,4
    a4.RemoveAt(1); // 2,4
    a4.RemoveAt(1); // 2
    a4.RemoveAll();

    // Query elements.
    Array<int> a5(5, cstyle);
    a5.At(0); // 1
    a5.At(4) += 1; // 6
    a5.Contains(4); // true
    a5.Contains(5); // false
    a5.IsEmpty(); // false

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

    // Insert elements at position.
    l1.InsertAfter(l1.FindElementPos(2), 3); // 2,3,4
    l1.InsertBefore(l1.FindIndexPos(0), 1); // 1,2,3,4

    // Remove elements at position.
    l1.RemoveAt(l1.FindIndexPos(1)); // 1,3,4

    // Remove elements at boundary.
    l1.RemoveHead(); // 3,4
    l1.RemoveTail(); // 4
    l1.RemoveAll();

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
    
    // Enumerate elements.
    List<int> l3;
    l3.AddTail(1);
    l3.AddTail(2);
    l3.AddTail(3);
    for (auto p = l3.HeadPos(); p; p = l3.FindNextPos(p))
    {
        l3.At(p);
    }
}

#endif

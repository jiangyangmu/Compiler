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
    a4.Insert(0, 2); // 2
    a4.Insert(1, 4); // 2,4
    a4.Insert(0, 1); // 1,2,4
    a4.Insert(2, 3); // 1,2,3,4

    // Remove elements at index.
    a4.Remove(0); // 2,3,4
    a4.Remove(1); // 2,4
    a4.Remove(1); // 2
    a4.RemoveAll();

    // Query elements.
    Array<int> a5(5, cstyle);
    a5.At(0); // 1
    a5.At(4) += 1; // 6
    a5.Contains(4); // true
    a5.Contains(5); // false
    a5.Empty(); // false

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

#endif
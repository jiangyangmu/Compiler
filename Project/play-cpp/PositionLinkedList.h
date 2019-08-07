#pragma once

class PositionLinkedList
{
public:
    class Iterator {};

    class Position
    {
        bool  HasValue();

        // assert: has value
        int & GetValue();
        void  SetValue(int i);
    };

    Iterator Begin();
    Iterator End();

    Position BeginPos();
    Position EndPos();

    // return: end pos, or pos before x (i <= x)
    Position FindPos(int i);
    // return: before pos
    // assert: before pos exists (pos != begin pos)
    Position FindPosBefore(Position pos);

    void Insert(int i);
    // assert: pos has value
    int  Remove(Position pos);
};

/*

    next = list.FindPos(i);
    if (next != list.BeginPos())
    {
        prev = list.FindPosBefore(next);
    }

    if (next.HasValue() && ...test value...)
    {
        value = list.Remove(next);
    }
*/
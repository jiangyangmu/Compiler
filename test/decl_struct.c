// simple struct
struct simple
{
    int i;
};

// self references
struct Node
{
    struct Node *next;
};
struct FN
{
    struct FN (*p)();
};

// struct members: css scope
struct _s
{
    int i;
    struct _ss
    {
        int i;
        struct _sss
        {
            int i;
        } _sssi;
    } _ssi;
} _si;

// struct tag: css scope
// EXPECT: 1 1 1 1 2 2 2 4 4 4 8 8
struct A _a1;
struct A
{
    char c;
} a1;
struct A a1_;
void test_tag()
{
    struct A _a2;
    struct A
    {
        short s;
    } a2;
    struct A a2_;
    {
        struct A _a3;
        struct A
        {
            int i;
        } a3;
        struct A a3_;
        {
            struct A _a4;
            struct A
            {
                int i1, i2;
            } a4;
            struct A a4_;
        }
    }
}

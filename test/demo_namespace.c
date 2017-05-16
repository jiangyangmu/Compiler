// unit test for namespace & scope of C Symbol System

// label: function scope
// EXPECT: "Duplicate label name..."
void test_label1()
{
	int a;
L: 	a = 1;
L: 	a = 2;
}
// EXPECT: "Duplicate label name..."
void test_label2()
{
	int a;
L: 	a = 1;
	{
L: 		a = 2;
	}
}
// EXPECT: "Duplicate label name..."
void test_label3()
{
	int a;
	{{
L: 		a = 1;
		{{{
L: 			a = 2;
		}}}
	}}
}
// EXPECT: "Duplicate label name..."
void test_label4()
{
	int a;
	{
		{
L: 			a = 1;
		}
		{
L: 			a = 2;
		}
	}
}

// struct/union members: css scope
struct _s {
	int i;
	struct {
		int i;
		struct {
			int i;
		} _sss;
	} _ss;
};

// struct/union/enum tag: css scope
// EXPECT: "1 2 4 8"
struct A { char c; } a1;
void test_tag()
{
	struct A { short s; } a2;
	{
		struct A { int i; } a3;
		{
			struct A { int i1, i2; } a4;
			printf("%d %d %d %d\n",
				sizeof(a1),
				sizeof(a2),
				sizeof(a3),
				sizeof(a4));
		}
	}
}

// global supports forward declaration, local do NOT ?
// EXPECT: "1 1 1 1 2 2 2 3 3 3 4 4"
struct A _a1;
struct A { char c; } a1;
struct A a1_;
void test_tag()
{
    struct A _a2;
    struct A { short s; } a2;
    struct A a2_;
    {
        struct A _a3;
        struct A { int i; } a3;
        struct A a3_;
        {
            struct A _a4;
            struct A { int i1, i2; } a4;
            struct A a4_;
            printf("%lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu %lu\n",
                    sizeof(_a1), sizeof(a1), sizeof(a1_),
                    sizeof(_a2), sizeof(a2), sizeof(a2_),
                    sizeof(_a3), sizeof(a3), sizeof(a3_),
                    sizeof(_a4), sizeof(a4), sizeof(a4_));
        }
    }
}


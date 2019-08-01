#include "../Source/Testing/Tester.h"

#include "../Source/Memory/Span.h"

using namespace LowLevel;

typedef std::vector<std::pair<size_t, const void *>> SpanVector;

#define EXPECT_SPAN(actual, expect) do \
{ \
    SpanVector v1(expect); \
    auto v2 = (actual); \
    auto i1 = v1.begin(); \
    auto i2 = v2.begin(); \
    bool matched = true; \
    while (matched && i1 != v1.end() && i2 != v2.end()) \
    { \
        matched = (i1->first == (*i2).nPage) && (i1->second == (*i2).cpvMemBegin); \
        ++i1, ++i2; \
    } \
    if (!matched || i1 != v1.end() || i2 != v2.end()) \
    { \
        std::cerr << "Wrong span list at " << __FILE__ << ":" << __LINE__ << std::endl;  \
        std::cerr << "Expect:\n"; \
        for (auto i : v1) std::cerr << "\t{ " << i.first << ", " << (void *)i.second << " }\n"; \
        std::cerr << std::endl; \
        std::cerr << "Actual:\n"; \
        for (auto i : v2) std::cerr << "\t{ " << i.nPage << ", " << i.cpvMemBegin << " }\n"; \
        std::cerr << std::endl; \
        TestRunner::Get().SetError(true); \
    } \
} while (false);

/*
    Test Cases

    1. Span allocator create/destroy (verify: SCB, span tree)

    2. Span alloc/free (verify: SCB, span tree, pvMemBegin)
        - 16 page, alloc 1 page * 15, free 0~14
        - 16 page, alloc 1 page * 15, free 14~0

    3. Span read/write

    4. OOM
        - 16 page, alloc 16 page * 1, OOM

    5. Lazy initialization

*/

TEST(SpanAllocator_Create)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    void * vPage[16];
    {
        char * pcMemBegin = (char *)sa.MemBegin();
        for (int i = 0; i < 16; ++i)
            vPage[i] = (void *)(pcMemBegin + i * PAGE_SIZE);
    }

    EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
        {1, vPage[14]},
        {2, vPage[12]},
        {4, vPage[8]},
        {8, vPage[0]},
    }));
}

TEST(SpanAllocator_AllocFree)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    void * vPage[16];
    {
        char * pcMemBegin = (char *)sa.MemBegin();
        for (int i = 0; i < 16; ++i)
            vPage[i] = (void *)(pcMemBegin + i * PAGE_SIZE);
    }

    EXPECT_EQ(sa.Alloc(1), vPage[14]);
    EXPECT_EQ(sa.Alloc(1), vPage[12]);
    EXPECT_EQ(sa.Alloc(1), vPage[13]);
    EXPECT_EQ(sa.Alloc(1), vPage[8]);
    EXPECT_EQ(sa.Alloc(1), vPage[9]);
    EXPECT_EQ(sa.Alloc(1), vPage[10]);
    EXPECT_EQ(sa.Alloc(1), vPage[11]);
    EXPECT_EQ(sa.Alloc(1), vPage[0]);
    EXPECT_EQ(sa.Alloc(1), vPage[1]);
    EXPECT_EQ(sa.Alloc(1), vPage[2]);
    EXPECT_EQ(sa.Alloc(1), vPage[3]);
    EXPECT_EQ(sa.Alloc(1), vPage[4]);
    EXPECT_EQ(sa.Alloc(1), vPage[5]);
    EXPECT_EQ(sa.Alloc(1), vPage[6]);
    EXPECT_EQ(sa.Alloc(1), vPage[7]);
    {
        sa.Free(vPage[0], 1);
        
        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[1], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[2], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[2]},
            {2, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[3], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[4], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[4]},
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[5], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[4]},
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[6], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[6]},
            {2, vPage[4]},
            {4, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[7], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[8], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[9], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[10], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[10]},
            {2, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[11], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[12], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[13], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {2, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
    {
        sa.Free(vPage[14], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }

    EXPECT_EQ(sa.Alloc(1), vPage[14]);
    EXPECT_EQ(sa.Alloc(1), vPage[12]);
    EXPECT_EQ(sa.Alloc(1), vPage[13]);
    EXPECT_EQ(sa.Alloc(1), vPage[8]);
    EXPECT_EQ(sa.Alloc(1), vPage[9]);
    EXPECT_EQ(sa.Alloc(1), vPage[10]);
    EXPECT_EQ(sa.Alloc(1), vPage[11]);
    EXPECT_EQ(sa.Alloc(1), vPage[0]);
    EXPECT_EQ(sa.Alloc(1), vPage[1]);
    EXPECT_EQ(sa.Alloc(1), vPage[2]);
    EXPECT_EQ(sa.Alloc(1), vPage[3]);
    EXPECT_EQ(sa.Alloc(1), vPage[4]);
    EXPECT_EQ(sa.Alloc(1), vPage[5]);
    EXPECT_EQ(sa.Alloc(1), vPage[6]);
    EXPECT_EQ(sa.Alloc(1), vPage[7]);
    {
        sa.Free(vPage[14], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
        }));
    }
    {
        sa.Free(vPage[13], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[13]},
            {1, vPage[14]},
        }));
    }
    {
        sa.Free(vPage[12], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[11], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[11]},
            {1, vPage[14]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[10], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[10]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[9], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[9]},
            {1, vPage[14]},
            {2, vPage[10]},
            {2, vPage[12]},
        }));
    }
    {
        sa.Free(vPage[8], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[7], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[7]},
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[6], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[6]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[5], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[5]},
            {1, vPage[14]},
            {2, vPage[6]},
            {2, vPage[12]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[4], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[3], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[3]},
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[2], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[2]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[1], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[1]},
            {1, vPage[14]},
            {2, vPage[2]},
            {2, vPage[12]},
            {4, vPage[4]},
            {4, vPage[8]},
        }));
    }
    {
        sa.Free(vPage[0], 1);

        EXPECT_SPAN(SpanView<SpanAllocator>(sa), (SpanVector{
            {1, vPage[14]},
            {2, vPage[12]},
            {4, vPage[8]},
            {8, vPage[0]},
        }));
    }
}

TEST(SpanAllocator_ReadWrite)
{
    SpanAllocator sa = CreateSpanAllocator(16);

    char * pcPageBegin = (char *)sa.Alloc(1);
    char * pcPageEnd = pcPageBegin + PAGE_SIZE;

    for (char * pc = pcPageBegin; pc < pcPageEnd; ++pc)
        *pc = 1;
    for (char * pc = pcPageBegin; pc < pcPageEnd; ++pc)
        EXPECT_EQ((int)*pc, 1);
}

TEST(SpanAllocator_OOM)
{
    SpanAllocator sa = CreateSpanAllocator(16);
    
    EXPECT_EQ(sa.Alloc(16), (void *)nullptr);
    
    EXPECT_NE(sa.Alloc(8), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(8), (void *)nullptr);

    EXPECT_NE(sa.Alloc(4), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(4), (void *)nullptr);

    EXPECT_NE(sa.Alloc(2), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(2), (void *)nullptr);

    EXPECT_NE(sa.Alloc(1), (void *)nullptr);
    EXPECT_EQ(sa.Alloc(1), (void *)nullptr);
}

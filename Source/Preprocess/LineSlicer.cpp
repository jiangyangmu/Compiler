#include "LineSlicer.h"

#include "../Base/ErrorHandling.h"

enum LineBlockType
{
    NONE,
    END,
    NEW_LINE,
    MERGE_LINE,
    CONTENT,
};

LineBlockType
NextASCIILineBlock(const char ** ppcBlkBegin,
                   const char ** ppcBlkEnd)
{
    LineBlockType lbt;
    const char * pc;
    bool bDone;

    lbt     = NONE;
    pc      = *ppcBlkEnd;
    bDone   = false;

    while (!bDone)
    {
        ASSERT(lbt == NONE || lbt == CONTENT);
        switch (*pc)
        {
            default:
                lbt = CONTENT;
                ++pc;
                break;
            case 0:
                if (lbt == NONE)
                    lbt = END;
                bDone = true;
                break;
            case '\n':
                if (lbt == NONE)
                    lbt = NEW_LINE, ++pc;
                bDone = true;
                break;
            case '\r':
                if (*(pc + 1) == '\n')
                {
                    if (lbt == NONE)
                        lbt = NEW_LINE, pc += 2;
                    bDone = true;
                }
                else
                {
                    lbt = CONTENT;
                    ++pc;
                }
                break;
            case '\\':
                if (*(pc + 1) == '\n')
                {
                    if (lbt == NONE)
                        lbt = MERGE_LINE, pc += 2;
                    bDone = true;
                }
                else
                {
                    lbt = CONTENT;
                    ++pc;
                }
                break;
        }
    }

    *ppcBlkBegin = *ppcBlkEnd;
    *ppcBlkEnd   = pc;

    ASSERT(lbt != NONE);
    return lbt;
}

std::vector<std::string>
ASCIILineSlice(std::string text)
{
    std::vector<std::string> vstrLogicalLines;
    std::string strLogicalLine;
    
    int nPhysicalLine = 1;
    int nPhysicalLineFix = 0;

    LineBlockType lbt;
    const char * pcBlkBegin;
    const char * pcBlkEnd;

    pcBlkBegin  = nullptr;
    pcBlkEnd    = text.data();

    do
    {
        lbt = NextASCIILineBlock(&pcBlkBegin, &pcBlkEnd);
        switch (lbt)
        {
            case END:
            case CONTENT:
                strLogicalLine.append(pcBlkBegin, pcBlkEnd);
                break;
            case NEW_LINE:
                ++nPhysicalLine;
                vstrLogicalLines.emplace_back(std::move(strLogicalLine));

                if (nPhysicalLineFix > 0)
                {
                    nPhysicalLine   += nPhysicalLineFix;
                    nPhysicalLineFix = 0;
                    vstrLogicalLines.emplace_back(std::string("#line ") + std::to_string(nPhysicalLine));
                }
                break;
            case MERGE_LINE:
                strLogicalLine.push_back(' ');
                ++nPhysicalLineFix;
                break;
            default:
                ASSERT(false);
                break;
        }
    }
    while (lbt != END);

    return vstrLogicalLines;
}

#ifdef UNIT_TEST
#include "../UnitTest/UnitTest.h"

TEST(LineSlicer_Simple)
{
    std::string in = (
        "line 1\n"
        "line 2\r\n"
        "line\\\n"
        "3\n"
        "line 5\n"
        "\n"
        );

    std::vector<std::string> out = {
        "line 1",
        "line 2",
        "line 3",
        "#line 5",
        "line 5",
        "",
    };

    auto lines = ASCIILineSlice(in);
    EXPECT_EQ(lines.size(), out.size());
    for (size_t i = 0; i < out.size(); ++i)
    {
        EXPECT_EQ(lines[i], out[i]);
    }
}

#endif
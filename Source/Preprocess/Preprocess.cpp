#include "Preprocess.h"

#include "../Util/Common.h"
#include "../Util/Charset.h"

namespace Preprocess {

void CharsetCheck(const ByteArray & input)
{
    bool good = true;

    for (char c : input)
    {
        if (!IsCSourceChar(c))
        {
            std::cout << "Bad char: " << std::hex << c << std::endl;
            good = false;
        }
    }

    ASSERT(good);
}

// Remove /* ... */, keep new line.
ByteArray RemoveComments(const ByteArray & input)
{
    ByteArray output;

    output.Reserve(input.Size());

    char * out = output.RawData();

    bool inComment = false;
    const char * in = input.RawData();
    const char * inEnd = input.RawData() + input.Size();
    while (in < inEnd)
    {
        // toggle in comment state
        if (in + 1 < inEnd)
        {
            if (*in == '/' && *(in + 1) == '*')
            {
                inComment = true;
                in += 2;
                continue;
            }
            else if (*in == '*' && *(in + 1) == '/')
            {
                inComment = false;
                in += 2;
                continue;
            }
        }

        if (inComment)
        {
            // keep new line in comment
            if (*in == '\n')
            {
                *out++ = *in++;
            }
        }
        else
        {
            *out++ = *in++;
        }
    }

    ASSERT(!inComment);

    output.SetSize(out - output.RawData());

    return output;
}

SourceContext Preprocess(const ByteArray & input)
{
    // 1. ByteArray => check charset => ByteArray
    // 2. ByteArray => remove comment (keep NL) => ByteArray
    // 3. ByteArray => break into line with format (char*, NL) => SourceContext
    CharsetCheck(input);

    ByteArray inputWithoutComment = RemoveComments(input);

    SourceContext sourceContext;
    
    ByteArray sourceLine;
    for (char byte : inputWithoutComment)
    {
        sourceLine.PushBack(byte);
        if (byte == '\n')
        {
            sourceContext.lines.emplace_back(std::move(sourceLine));
            ASSERT(sourceLine.Empty());
        }
    }
    if (!sourceLine.Empty())
    {
        if (sourceLine.Last() != '\n')
            sourceLine.PushBack('\n');

        sourceContext.lines.emplace_back(std::move(sourceLine));
        ASSERT(sourceLine.Empty());
    }

    return sourceContext;
}

}

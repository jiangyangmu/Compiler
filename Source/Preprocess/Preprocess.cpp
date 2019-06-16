#include "Preprocess.h"

#include "../Util/Common.h"
#include "../Util/Charset.h"

#include <fstream>

namespace Preprocess {

std::string GetFileContent(const char * fileName)
{
    std::ifstream ifs(fileName, std::ifstream::in);
    //ifs.seekg(3);
    return std::string(std::istreambuf_iterator<char>(ifs),
                       std::istreambuf_iterator<char>());
}

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
            else
            {
                ++in;
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
            if (StringRef(sourceLine.RawData(), 8) == "#include")
            {
                const char * begin, * end;
                std::string includedFileContent;

                begin = sourceLine.RawData() + 8;
                while (*begin != '"') ++begin;
                end = ++begin;
                while (*end != '"') ++end;

                includedFileContent = GetFileContent(StringRef(begin, end - begin).toString().c_str());
                
                ByteArray includedSourceCode(includedFileContent.data(), includedFileContent.size());
                SourceContext includedContext = Preprocess(includedSourceCode);
                sourceContext.lines.insert(sourceContext.lines.end(),
                                           includedContext.lines.begin(),
                                           includedContext.lines.end());
                sourceLine.Clear();
            }
            else
            {
                sourceContext.lines.emplace_back(std::move(sourceLine));
                ASSERT(sourceLine.Empty());
            }
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

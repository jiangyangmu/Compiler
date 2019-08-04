#pragma once

#include "../Base/String.h"

#include <vector>

namespace Preprocess {

struct SourceContext
{
    std::vector<ByteArray> lines;
};

SourceContext Preprocess(const ByteArray & input);

}

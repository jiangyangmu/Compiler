#pragma once

#include "../Util/String.h"

#include <vector>

namespace Preprocess {

struct SourceContext
{
    std::vector<ByteArray> lines;
};

SourceContext Preprocess(const ByteArray & input);

}

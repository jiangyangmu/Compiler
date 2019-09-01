#pragma once

#include "../Base/String.h"

#include <string>
#include <vector>

struct MatchResult
{
    size_t offset;
    size_t length; // 0: no match
    size_t which;  // 0: no match, 1: match 1st pattern, ...
};

std::vector<MatchResult> Match(std::vector<std::string> patterns, StringRef text);

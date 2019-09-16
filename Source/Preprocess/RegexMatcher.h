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

struct MatchEngine
{
    struct Dfa * dfa;
};

// Compile
MatchEngine Compile(std::vector<std::string> & patterns);

// Match one
MatchResult MatchPrefix(MatchEngine m, StringView text);
// Match all
std::vector<MatchResult> MatchAll(MatchEngine m, StringView text);
std::vector<MatchResult> MatchAll(std::vector<std::string> patterns, StringView text);

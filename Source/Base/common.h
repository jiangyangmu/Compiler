#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

#define ELEMENT_COUNT(a) (sizeof(a) / sizeof((a)[0]))

#include "String.h"

#define CHECK(x) assert(x)
#define CHECK_GT(a, b) assert((a) > (b))
#define CHECK_EQ(a, b) assert((a) == (b))

#include "ErrorHandling.h"

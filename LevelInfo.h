#pragma once

#include "Usings.h"

// Struct to represent an order
struct LevelInfo
{
    Price price_;
    Quantity quantity_;
};
using LevelInfos = vector<LevelInfo>;
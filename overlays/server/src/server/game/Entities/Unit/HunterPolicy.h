#pragma once
#include <cstdint>
namespace HunterCombat {
// Pure decision shared by server adapter and exhaustive relationship tests.
constexpr int RelationIds(bool hunterA, bool hunterB, uint64_t targetA, uint64_t targetB, uint64_t playerA, uint64_t playerB, bool same)
{
    if (hunterA) return !same && targetA && playerB==targetA ? 1 : 0;
    if (hunterB) return !same && targetB && playerA==targetB ? 1 : 0;
    return -1;
}
}

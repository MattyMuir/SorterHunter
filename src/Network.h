#pragma once
#include <cstdint>
#include <vector>

struct CE
{
	uint8_t lo, hi;
	auto operator<=>(const CE& other) const = default;
};

using Network = std::vector<CE>;

Network Concatenate(const Network& a, const Network& b);
uint32_t ComputeDepth(const Network& network);
void PrintNetwork(const Network& network);
#pragma once
#include <cstdint>

#include "types.h"

class SorterHunter
{
public:
	SorterHunter(uint8_t N_, const Network& prefix_);

	void Hunt();

protected:
	uint8_t N;
	Network prefix;
};
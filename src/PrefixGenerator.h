#pragma once
#include "Network.h"

enum PrefixType { PrefixNone, PrefixFixed, PrefixGreedy, PrefixHybrid };

class PrefixGenerator
{
public:
	PrefixGenerator(PrefixType type_, const std::vector<CE>& alphabet_);

	Network Generate() const;

protected:
	// Constructor parameters
	PrefixType type;
	const std::vector<CE> alphabet;

	// Config parameters
	uint8_t N;
	bool symmetric;
	uint32_t greedyPrefixSize;
};
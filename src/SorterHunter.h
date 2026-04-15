#pragma once
#include "Types/Network.h"
#include "Types/ConvexHull.h"
#include "Types/types.h"
#include "Prefix/PrefixGenerator.h"

class SorterHunter
{
public:
	SorterHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_);

	virtual void StartHunting(size_t maxEpochs = 0) = 0;
	virtual void StopHunting() = 0;

protected:
	// Constructor parameters
	const PrefixGenerator& prefixGenerator;
	const Network postfix;
	const std::vector<CE> alphabet;

	// Config parameters
	const uint8_t N;
	const bool symmetric;

	// State
	ConvexHull convexHull;
	BitParallelList testVectors;
	Network prefix;

	void PrepareTestVectors();
	static void RunNetwork(const Network& network, BPWord* inputs);
	bool TestNetwork(const Network& network, SortWord* problemOutput = nullptr);
	void RegisterValidCore(Network& networkCore);
	Network SymmetricExpansion(const Network& network) const;
	Network SanitizeNetwork(const Network network) const;
};
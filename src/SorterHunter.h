#pragma once
#include <cstdint>

#include "types.h"
#include "Network.h"
#include "NetworkMutator.h"
#include "ConvexHull.h"
#include "PrefixGenerator.h"

class SorterHunter
{
public:
	SorterHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_);

	void Hunt(size_t epochs = 0);

protected:
	// Constructor parameters
	const PrefixGenerator& prefixGenerator;
	const Network postfix;
	const std::vector<CE> alphabet;

	// Config parameters
	const uint8_t N;
	const bool symmetric;
	const uint32_t maxMutations, escapeRate, restartRate;

	// State
	size_t epoch = 0;
	Network prefix;
	NetworkMutator mutator;
	ConvexHull convexHull;
	BitParallelList testVectors;
	Network networkCore;

	void PrepareTestVectors();
	void ProduceInitialSolution();
	static void RunNetwork(const Network& network, BPWord* inputs);
	std::pair<bool, SortWord> TestNetwork(const Network& network);
	bool TestNetworkAndBump(const Network& network);
	void BumpVectorPosition(size_t groupIdx, size_t bitIdx);
	void LogEpoch();
	void RegisterValidCore();
	void UphillStep();
	void Restart();
};
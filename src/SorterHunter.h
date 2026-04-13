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

	void Hunt(size_t maxEpochs = 0);

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
	NetworkMutator mutator;
	ConvexHull convexHull;
	std::vector<BitParallelList> allTestVectors;
	std::vector<Network> prefixes, networkCores;

	void HuntWorker(size_t threadIdx, size_t maxEpochs);

	BitParallelList PrepareTestVectors(const Network& prefix);
	void ProduceInitialSolution(size_t threadIdx);
	static void RunNetwork(const Network& network, BPWord* inputs);
	std::pair<bool, SortWord> TestNetwork(const Network& network, const BitParallelList& testVectors);
	bool TestNetworkAndBump(const Network& network, const BitParallelList& testVectors);
	void BumpVectorPosition(size_t groupIdx, size_t bitIdx);
	void LogEpoch(size_t threadIdx, size_t epoch);
	void RegisterValidCore(size_t threadIdx);
	void UphillStep(Network& networkCore);
};
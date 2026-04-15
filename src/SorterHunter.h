#pragma once
#include <cstdint>
#include <thread>

#include "types.h"
#include "Network.h"
#include "NetworkMutator.h"
#include "ConvexHull.h"
#include "PrefixGenerator.h"

class SorterHunter
{
public:
	SorterHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_);

	void StartHunting(size_t maxEpochs = 0);
	void StopHunting();
	bool HasFoundNetwork() const;
	Network GetSmallestNetwork() const;

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
	BitParallelList testVectors;
	Network prefix;
	std::vector<Network>networkCores;

	bool shouldStop = false;
	std::vector<std::thread> workers;

	void HuntWorker(size_t threadIdx, size_t maxEpochs);

	void PrepareTestVectors();
	void ProduceInitialSolution(Network& networkCore);
	static void RunNetwork(const Network& network, BPWord* inputs);
	std::pair<bool, SortWord> TestNetwork(const Network& network);
	bool TestNetworkAndBump(const Network& network);
	void BumpVectorPosition(size_t groupIdx, size_t bitIdx);
	void LogEpoch(size_t threadIdx, size_t epoch);
	void RegisterValidCore(Network& networkCore);
	void UphillStep(Network& networkCore);
	Network SanitizeNetwork(const Network network);
};
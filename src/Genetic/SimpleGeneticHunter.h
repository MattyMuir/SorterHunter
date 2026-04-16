#pragma once
#include <cstdint>
#include <thread>

#include "Types/types.h"
#include "Types/Network.h"
#include "NetworkMutator.h"
#include "Types/ConvexHull.h"
#include "Prefix/PrefixGenerator.h"
#include "SorterHunter.h"

class SimpleGeneticHunter : public SorterHunter
{
public:
	SimpleGeneticHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_);

	void StartHunting(size_t maxEpochs) override;
	void StopHunting() override;

protected:
	// Config parameters
	const uint32_t maxMutations, escapeRate, restartRate;

	// State
	NetworkMutator mutator;
	std::vector<Network> networkCores;
	bool shouldStop = false;
	std::vector<std::thread> workers;

	void HuntWorker(size_t threadIdx, size_t maxEpochs);
	void ProduceInitialSolution(Network& networkCore);
	void LogEpoch(size_t threadIdx, size_t epoch);
	void UphillStep(Network& networkCore);
};
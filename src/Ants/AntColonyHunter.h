#pragma once
#include <thread>

#include "SorterHunter.h"
#include "CEGraph.h"

class AntColonyHunter : public SorterHunter
{
protected:
	using AntPath = std::vector<std::vector<size_t>>;

public:
	AntColonyHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_);

	void StartHunting(size_t maxEpochs) override;
	void StopHunting() override;

protected:
	std::vector<CEGraph> layerGraphs;

	bool shouldStop = false;
	std::vector<std::thread> workers;

	float alpha = 1.6;

	void HuntWorker();
	AntPath RandomAntPath();
	Network PathToNetworkCore(const AntPath& path);
	void EvaporatePheromone(float factor);
	void DepositPheromone(const AntPath& path, float amount);
};
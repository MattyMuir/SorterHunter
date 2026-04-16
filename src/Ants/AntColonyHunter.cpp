#include "AntColonyHunter.h"

#include <bit>
#include <ranges>

#include "Utility/print.h"

AntColonyHunter::AntColonyHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_)
	: SorterHunter(prefixGenerator_, postfix_, alphabet_)
{}

void AntColonyHunter::StartHunting(size_t maxEpochs)
{
	workers.emplace_back([this]() { this->HuntWorker(); });
}

void AntColonyHunter::StopHunting()
{
	shouldStop = true;
	for (auto& worker : workers) worker.join();
}

static inline uint32_t CeilLog2(uint32_t x)
{
	return std::bit_width(x - 1);
}

void AntColonyHunter::HuntWorker()
{
	prefix = {};
	PrepareTestVectors();

	// Build layer graphs
	CEGraph layerGraph = BuildCEGraph(N, symmetric, alphabet);
	uint32_t Nlog2 = CeilLog2(N);
	uint32_t depthBound = 9;
	layerGraphs.assign(depthBound, layerGraph);

	while (!shouldStop)
	{
		struct Score
		{
			float sorted; size_t size;
			auto operator<=>(const Score& other) const = default;
		};
#if 0
		std::vector<std::pair<AntPath, Score>> paths;
		for (size_t antIdx = 0; antIdx < 100; antIdx++)
		{
			auto path = RandomAntPath();

			// Score path
			Network networkCore = PathToNetworkCore(path);
			Network se = symmetric ? SymmetricExpansion(networkCore) : networkCore;
			float unsortedFrac = (float)UnsortedFraction(se);

			if (unsortedFrac == 0)
				RegisterValidCore(networkCore);

			Score score{ 1.0f - unsortedFrac, se.size() };
			paths.push_back({ path, score });
		}

		// Deposit pheromone
		size_t numDeposits = 3;

		//for (auto& graph : layerGraphs) graph.ScaleWeights(0.95f);

		std::sort(paths.begin(), paths.end(), [](const auto& pair1, const auto& pair2) { return pair1.second > pair2.second; });
		PRINT("Sorted: {}\n", paths[0].second.sorted);
		for (size_t pathIdx = 0; pathIdx < numDeposits; pathIdx++)
			DepositPheromone(paths[pathIdx].first, numDeposits - pathIdx);
#else
		std::vector<std::pair<AntPath, float>> paths;
		for (size_t antIdx = 0; antIdx < 5000; antIdx++)
		{
			auto path = RandomAntPath();

			// Score path
			Network networkCore = PathToNetworkCore(path);
			Network se = symmetric ? SymmetricExpansion(networkCore) : networkCore;
			size_t numInversions = NumInversions(se);
			if (antIdx == 0)
				PRINT("Inversions: {}\n", numInversions);

			float score = 1.0f / std::log(numInversions + 2);

			paths.push_back({ path, score });
			if (numInversions == 0)
				RegisterValidCore(networkCore);
		}

		// Evaporate pheromone
		//for (auto& graph : layerGraphs) graph.ScaleWeights(0.99f);

		// Deposit pheromone
		std::sort(paths.begin(), paths.end(), [](const auto& pair1, const auto& pair2) { return pair1.second > pair2.second; });
		for (size_t rank = 0; rank < 1; rank++)
			DepositPheromone(paths[rank].first, paths[rank].second);
			
#endif
	}
}

std::vector<std::vector<size_t>> AntColonyHunter::RandomAntPath()
{
	size_t layerIdx = 0;
	size_t vertexIdx = layerGraphs[0].StartIndex();

	size_t numCEs = 0;
	std::vector<std::vector<size_t>> path{ {} };
	for (;;)
	{
		// Take a random step
		vertexIdx = layerGraphs[layerIdx].WeightedRandomNeighbour(vertexIdx);

		// If the ant took a step to a regular vertex, add to the path and continue
		if (vertexIdx != layerGraphs[layerIdx].EndIndex())
		{
			path[layerIdx].push_back(vertexIdx);
			numCEs++;
			if (numCEs >= 26) break;
			continue;
		}

		// Ant has reached the end of the layer
		layerIdx++;
		if (layerIdx >= layerGraphs.size()) break;
		vertexIdx = layerGraphs[layerIdx].StartIndex();
		path.push_back({});
	}

	return path;
}

Network AntColonyHunter::PathToNetworkCore(const AntPath& path)
{
	Network networkCore;
	for (const auto& layer : path)
		for (size_t ceIdx : layer)
			networkCore.push_back(alphabet[ceIdx]);
	return networkCore;
}

void AntColonyHunter::DepositPheromone(const AntPath& path, float amount)
{
	for (size_t layerIdx = 0; layerIdx < path.size(); layerIdx++)
	{
		const std::vector<size_t>& layer = path[layerIdx];
		CEGraph& graph = layerGraphs[layerIdx];

		for (size_t stepIdx = 0; stepIdx < layer.size(); stepIdx++)
		{
			size_t ceIdx0 = (stepIdx == 0) ? graph.StartIndex() : layer[stepIdx - 1];
			size_t ceIdx1 = layer[stepIdx];
			float weight = graph.ReadWeight(ceIdx0, ceIdx1);
			float pheromone = std::pow(weight, 1.0f / alpha) + amount;
			float newWeight = std::pow(pheromone, alpha);
			graph.SetWeight(ceIdx0, ceIdx1, newWeight);
		}

		size_t ceIdx0 = layer.back();
		size_t ceIdx1 = graph.EndIndex();
		float weight = graph.ReadWeight(ceIdx0, ceIdx1);
		float pheromone = std::pow(weight, 1.0f / alpha) + amount;
		float newWeight = std::pow(pheromone, alpha);
		graph.SetWeight(ceIdx0, ceIdx1, newWeight);
	}
}
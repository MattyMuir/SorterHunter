#pragma once
#include <unordered_map>

#include "Types/Network.h"
#include "WeightedSampler.h"

class CEGraph
{
public:
	float ReadWeight(size_t ceIdx0, size_t ceIdx1) const;
	void SetWeight(size_t ceIdx0, size_t ceIdx1, float weight);
	size_t WeightedRandomNeighbour(size_t ceIdx) const;
	void ScaleWeights(float scale);

	size_t StartIndex() const;
	size_t EndIndex() const;

protected:
	std::vector<std::vector<size_t>> neighbours;
	std::vector<std::unordered_map<size_t, size_t>> localIndexMap;
	std::vector<WeightedSampler> samplers;

	size_t LocalIndex(size_t from, size_t to) const;

	friend CEGraph BuildCEGraph(uint8_t N, bool symmetric, const std::vector<CE>& alphabet);
};

CEGraph BuildCEGraph(uint8_t N, bool symmetric, const std::vector<CE>& alphabet);
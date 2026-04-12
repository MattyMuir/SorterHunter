#include "Network.h"

#include "types.h"
#include "print.h"

Network Concatenate(const Network& a, const Network& b)
{
	Network result{ a };
	result.insert(result.end(), b.begin(), b.end());
	return result;
}

uint32_t ComputeDepth(const Network& network)
{
	std::vector<SortWord> layers;
	int numLayers = 0;

	for (size_t k = 0; k < network.size(); k++)
	{
		uint32_t i = network[k].lo;
		uint32_t j = network[k].hi;
		int matchidx = numLayers;
		int idx = numLayers - 1;
		while (idx >= 0)
		{
			if ((layers[idx] & ((1ULL << i) | (1ULL << j))) == 0)
				matchidx = idx;
			else break;
			idx--;
		}
		if (matchidx >= numLayers)
		{
			layers.push_back(0);
			numLayers++;
		}
		layers[matchidx] |= 1ULL << i;
		layers[matchidx] |= 1ULL << j;
	}

	return numLayers;
}

void PrintNetwork(const Network& network)
{
	PRINT("[");
	for (size_t k = 0; k < network.size(); k++)
	{
		PRINT("({},{})", network[k].lo, network[k].hi);
		PRINT("{}", ((k + 1) < network.size()) ? ',' : ']');
	}
	PRINT("}}\r\n");
}
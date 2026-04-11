#pragma once
#include "types.h"
#include "ConfigParser.h"

class NetworkMutator
{
protected:
	enum MutationType : uint8_t
	{
		MutationRemovePair,
		MutationSwapPairs,
		MutationReplacePair,
		MutationCrossPairs,
		MutationSwapIntersectingPairs,
		MutationReplaceHalfPair,
		NMUTATIONTYPES
	};

public:
	NetworkMutator(const ConfigParser& cp, const std::vector<CE>& alphabet_);

	void MutateMulti(Network& network, size_t maxMutations);
	void MutateOnce(Network& network);

protected:
	const std::vector<CE>& alphabet;
	std::vector<MutationType> mutationSelector;

	bool TryMutate(Network& network);

	// Mutation types
	static bool RemovePair(Network& network);
	static bool SwapPairs(Network& network);
	bool ReplacePair(Network& network);
	static bool CrossPairs(Network& network);
	static bool SwapIntersectingPairs(Network& network);
	bool ReplaceHalfPair(Network& network);
};
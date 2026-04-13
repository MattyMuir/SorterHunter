#include "NetworkMutator.h"

#include "GlobalRandom.h"

NetworkMutator::NetworkMutator(const std::vector<CE>& alphabet_)
	: alphabet(alphabet_)
{
	uint32_t mutationWeights[NMUTATIONTYPES];
	mutationWeights[MutationRemovePair]				= Config::GetInt("WeightRemovePair", 1);
	mutationWeights[MutationSwapPairs]				= Config::GetInt("WeightSwapPairs", 1);
	mutationWeights[MutationReplacePair]			= Config::GetInt("WeightReplacePair", 1);
	mutationWeights[MutationCrossPairs]				= Config::GetInt("WeightCrossPairs", 1);
	mutationWeights[MutationSwapIntersectingPairs]	= Config::GetInt("WeightSwapIntersectingPairs", 1);
	mutationWeights[MutationReplaceHalfPair]		= Config::GetInt("WeightReplaceHalfPair", 1);

	// Build LUT for fast weighted selection
	for (uint8_t typeIdx = 0; typeIdx < NMUTATIONTYPES; typeIdx++)
	{
		uint32_t weight = mutationWeights[typeIdx];
		for (uint32_t k = 0; k < weight; k++)
			mutationSelector.push_back((MutationType)typeIdx);
	}
}

void NetworkMutator::MutateMulti(Network& network, size_t maxMutations)
{
	size_t numMutations = 1 + GlobalGen()() % maxMutations;
	for (size_t i = 0; i < numMutations; i++)
		MutateOnce(network);
}

void NetworkMutator::MutateOnce(Network& network)
{
	while (!TryMutate(network));
}

bool NetworkMutator::TryMutate(Network& network)
{
	MutationType type = RandomElement(mutationSelector);
	switch (type)
	{
		case MutationRemovePair:			return RemovePair(network);
		case MutationSwapPairs:				return SwapPairs(network);
		case MutationReplacePair:			return ReplacePair(network);
		case MutationCrossPairs:			return CrossPairs(network);
		case MutationSwapIntersectingPairs:	return SwapIntersectingPairs(network);
		case MutationReplaceHalfPair:		return ReplaceHalfPair(network);
		default: __builtin_unreachable();
	}
}

// Removal of random pair from list
bool NetworkMutator::RemovePair(Network& network)
{
	if (network.empty()) return false;

	size_t pairIdx = RandomIndex(network);
	network.erase(network.begin() + pairIdx);
	return true;
}

// Swap two pairs at random positions in list
bool NetworkMutator::SwapPairs(Network& network)
{
	if (network.size() < 2) return false;

	uint32_t a = RandomIndex(network);
	uint32_t b = RandomIndex(network);
	if (network[a] == network[b]) return false;
	if (a > b) std::swap(a, b);

	bool dependent = false;
	uint8_t alo = network[a].lo;
	uint8_t ahi = network[a].hi;
	uint8_t blo = network[b].lo;
	uint8_t bhi = network[b].hi;

	// Pairs should either intersect, or another pair should exist between them that uses
	// one of the same 4 inputs. Otherwise, comparisons can be executed in parallel and
	// swapping them has no effect. 
	if ((blo == alo) || (blo == ahi) || (bhi == alo) || (bhi == ahi))
		dependent = true;
	else
	{
		for (uint32_t k = a + 1; k < b; k++)
		{
			uint8_t clo = network[k].lo;
			uint8_t chi = network[k].hi;
			if ((clo == alo) || (clo == ahi) || (chi == alo) || (chi == ahi) ||
				(clo == blo) || (clo == bhi) || (chi == blo) || (chi == bhi))
			{
				dependent = true;
				break;
			}
		}
	}
	if (!dependent) return false;

	std::swap(network[a], network[b]);
	return true;
}

// Replace a pair at a random position with another random pair
bool NetworkMutator::ReplacePair(Network& network)
{
	if (network.empty()) return false;

	uint32_t a = RandomIndex(network);
	CE p = RandomElement(alphabet);
	bool ceIsDiff = (network[a] != p);
	network[a] = p;
	return ceIsDiff;
}

// Cross two pairs at random positions in list
bool NetworkMutator::CrossPairs(Network& network)
{
	if (network.size() < 2) return false;

	uint32_t a = RandomIndex(network);
	uint32_t b = RandomIndex(network);
	uint8_t alo = network[a].lo;
	uint8_t ahi = network[a].hi;
	uint8_t blo = network[b].lo;
	uint8_t bhi = network[b].hi;

	if ((alo == blo) || (alo == bhi) || (ahi == blo) || (ahi == bhi)) return false;

	uint32_t r2 = GlobalGen()() % 2;
	uint8_t x = r2 ? bhi : blo;
	uint8_t y = r2 ? blo : bhi;
	network[a].lo = std::min(alo, x);
	network[a].hi = std::max(alo, x);
	network[b].lo = std::min(ahi, y);
	network[b].hi = std::max(ahi, y);
	return true;
}

// Swap neighbouring intersecting pairs - special case of 'SwapPairs'
bool NetworkMutator::SwapIntersectingPairs(Network& network)
{
	if (network.size() < 2) return false;

	uint32_t a = RandomIndex(network);
	uint8_t alo = network[a].lo;
	uint8_t ahi = network[a].hi;
	for (uint32_t b = a + 1; b < network.size(); b++)
	{
		uint8_t blo = network[b].lo;
		uint8_t bhi = network[b].hi;
		if ((blo == alo) || (blo == ahi) || (bhi == alo) || (bhi == ahi))
		{
			if (network[a] != network[b])
			{
				std::swap(network[a], network[b]);
				return true;
			}
			break;
		}
	}

	return false;
}

// Change one half of a pair - special case of 'ReplacePair'
bool NetworkMutator::ReplaceHalfPair(Network& network)
{
	if (network.empty()) return false;

	uint32_t a = RandomIndex(network);
	CE p = network[a];
	CE q;
	do
	{
		q = RandomElement(alphabet);
	} while ((q.lo != p.lo) && (q.hi != p.lo) && (q.lo != p.hi) && (q.hi != p.hi));

	if (q == p) return false;
	network[a] = q;
	return true;
}
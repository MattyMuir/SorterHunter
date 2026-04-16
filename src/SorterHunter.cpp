#include "SorterHunter.h"

#include "Config/Config.h"
#include "Utility/print.h"
#include "Prefix/prefix_processor.h"
#include "Utility/GlobalRandom.h"

SorterHunter::SorterHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_)
	: prefixGenerator(prefixGenerator_),
	postfix(postfix_),
	alphabet(alphabet_),
	N(Config::GetInt("Ninputs")),
	symmetric(Config::GetInt("Symmetric", 0))
{}

bool SorterHunter::HasFoundNetwork() const
{
	return !convexHull.IsEmpty();
}

Network SorterHunter::GetSmallestNetwork() const
{
	return convexHull.GetSmallestNetwork();
}

void SorterHunter::PrepareTestVectors()
{
	if (Config::Verbosity() >= VerbosityHigh) PRINT("{:<50}\r", "Preparing test vectors...");

	// Compute all possible outputs from the prefix
	SinglePatternList singles;
	computePrefixOutputs(N, prefix, singles);

	// Shuffle test vectors to improve probability of early rejection of non-sorters
	std::shuffle(singles.begin(), singles.end(), GlobalGen());

	// Pack the possible prefix outputs into a bit-parallel list
	convertToBitParallel(N, singles, Config::GetInt("Symmetric"), testVectors);
}

void SorterHunter::RunNetwork(const Network & network, BPWord * inputs)
{
	for (auto [i, j] : network)
	{
		BPWord iold = inputs[i];
		inputs[i] = _mm256_and_si256(inputs[i], inputs[j]);
		inputs[j] = _mm256_or_si256(inputs[j], iold);
	}
}

static inline uint64_t CountrZero(__m256i x)
{
	__m256i zerowords = _mm256_cmpeq_epi32(x, _mm256_setzero_si256());
	uint32_t zeromask = ~_mm256_movemask_ps(_mm256_castsi256_ps(zerowords));
	uint32_t elems[8];
	_mm256_storeu_si256((__m256i*)elems, x);

	uint32_t wordIdx = std::countr_zero(zeromask);
	return wordIdx * 32 + std::countr_zero(elems[wordIdx]);
}

static inline uint64_t ReadBit(__m256i x, size_t pos)
{
	uint64_t xMem[4];
	_mm256_storeu_si256((__m256i*)xMem, x);
	return (xMem[pos / 64] >> (pos % 64)) & 1;
}

bool SorterHunter::TestNetwork(const Network& network, SortWord* problemOutput)
{
	for (size_t groupIdx = 0; groupIdx < testVectors.size() / N; groupIdx++)
	{
		// Copy this group into 'currentGroup'
		BPWord currentGroup[NMAX];
		memcpy(currentGroup, testVectors.data() + groupIdx * N, N * sizeof(BPWord));

		// Sort PARWORDSIZE inputs in parallel
		RunNetwork(network, currentGroup);

		// Scan for forbidden 1 -> 0 transition
		BPWord accum = _mm256_setzero_si256();
		for (size_t k = 0; k < (N - 1u); k++)
			accum = _mm256_or_si256(accum, _mm256_andnot_si256(currentGroup[k + 1], currentGroup[k]));

		// If found, determine which input within the group was incorrectly sorted
		if (!_mm256_testz_si256(accum, accum))
		{
			// Early exit if problem output has not been requested
			if (!problemOutput) return false;

			// Convert failing output into serial word
			size_t bitIdx = CountrZero(accum);
			SortWord output = 0;
			for (uint8_t inputIdx = 0; inputIdx < N; inputIdx++)
			{
				uint64_t bit = ReadBit(currentGroup[inputIdx], bitIdx);
				output |= bit << inputIdx;
			}

			*problemOutput = output;
			return false;
		}
	}

	return true;
}

static inline size_t Popcount(__m256i x)
{
	return std::popcount((uint64_t)_mm256_extract_epi64(x, 0))
		+ std::popcount((uint64_t)_mm256_extract_epi64(x, 1))
		+ std::popcount((uint64_t)_mm256_extract_epi64(x, 2))
		+ std::popcount((uint64_t)_mm256_extract_epi64(x, 3));
}

double SorterHunter::UnsortedFraction(const Network& network)
{
	size_t unsortedNum = 0;

	for (size_t groupIdx = 0; groupIdx < testVectors.size() / N; groupIdx++)
	{
		// Copy this group into 'currentGroup'
		BPWord currentGroup[NMAX];
		memcpy(currentGroup, testVectors.data() + groupIdx * N, N * sizeof(BPWord));

		// Sort PARWORDSIZE inputs in parallel
		RunNetwork(network, currentGroup);

		// Scan for forbidden 1 -> 0 transition
		BPWord accum = _mm256_setzero_si256();
		for (size_t k = 0; k < (N - 1u); k++)
			accum = _mm256_or_si256(accum, _mm256_andnot_si256(currentGroup[k + 1], currentGroup[k]));

		unsortedNum += Popcount(accum);
	}

	size_t numInputs = (testVectors.size() / N) * 256;
	return (double)unsortedNum / numInputs;
}

size_t SorterHunter::NumInversions(const Network& network)
{
	size_t numInversions = 0;

	for (size_t groupIdx = 0; groupIdx < testVectors.size() / N; groupIdx++)
	{
		// Copy this group into 'currentGroup'
		BPWord currentGroup[NMAX];
		memcpy(currentGroup, testVectors.data() + groupIdx * N, N * sizeof(BPWord));

		// Sort PARWORDSIZE inputs in parallel
		RunNetwork(network, currentGroup);

		// Count inversions
		BPWord accum = _mm256_setzero_si256();
		for (size_t k = 0; k < (N - 1u); k++)
			numInversions += Popcount(_mm256_andnot_si256(currentGroup[k + 1], currentGroup[k]));
	}

	return numInversions;
}

void SorterHunter::RegisterValidCore(Network& networkCore)
{
	// Concatenate with prefix and postfix and calculate properties
	Network totalNetwork = symmetric ? SymmetricExpansion(networkCore) : networkCore;
	totalNetwork = Concatenate(prefix, Concatenate(totalNetwork, postfix));

	// Check if this network improves the convex hull
	if (!convexHull.AddEntry(totalNetwork, networkCore)) return;

	// Reduce spam output by excluding neworks worse than bubblesort
	bool betterThanBubble = (totalNetwork.size() <= ((N * (N - 1)) / 2));
	if (!betterThanBubble && Config::Verbosity() < VerbosityHigh) return;

	// Print metadata and network
	PrintNetwork(totalNetwork);
	convexHull.Print();
}

Network SorterHunter::SymmetricExpansion(const Network& network) const
{
	Network symmetricNetwork;
	for (const CE& inpair : network)
	{
		symmetricNetwork.push_back(inpair);
		if (inpair.lo + inpair.hi == N - 1) continue;

		CE sp = { (uint8_t)(N - 1 - inpair.hi), (uint8_t)(N - 1 - inpair.lo) };
		symmetricNetwork.push_back(sp);
	}

	return symmetricNetwork;
}

Network SorterHunter::SanitizeNetwork(const Network network) const
{
	Network ret{ network };
	std::erase_if(ret, [this](CE ce)
		{
			if (ce.lo >= N || ce.hi >= N) return true;
			if (ce.lo >= ce.hi) return true;

			uint8_t loSym = N - 1 - ce.hi;
			uint8_t hiSym = N - 1 - ce.lo;
			CE ceSym{ loSym, hiSym };
			return ceSym < ce && symmetric;
		});
	return ret;
}
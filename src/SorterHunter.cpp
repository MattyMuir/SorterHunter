#include "SorterHunter.h"

#include <bit>
#include <thread>
#include <algorithm>

#include "version.h"
#include "print.h"
#include "Config.h"
#include "prefix_processor.h"
#include "GlobalRandom.h"
#include "utils.h"

SorterHunter::SorterHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_)
	: prefixGenerator(prefixGenerator_),
	postfix(postfix_),
	alphabet(alphabet_),
	N(Config::GetInt("Ninputs")),
	symmetric(Config::GetInt("Symmetric", 0)),
	maxMutations(Config::GetInt("MaxMutations", 1)),
	escapeRate(Config::GetInt("EscapeRate", 0)),
	restartRate(Config::GetInt("RestartRate", 0)),
	mutator(alphabet_) {}

void SorterHunter::Hunt(size_t maxEpochs)
{
	// Generate a prefix
	prefix = prefixGenerator.Generate();

	// Prepare the test vectors based on the prefix
	PrepareTestVectors();

	// === Launch worker threads ===
	size_t numThreads = std::thread::hardware_concurrency() - 1;
	networkCores.resize(numThreads, Network{});

	std::vector<std::thread> workers;
	workers.reserve(numThreads);
	for (size_t threadIdx = 0; threadIdx < numThreads; threadIdx++)
		workers.emplace_back([this, threadIdx, maxEpochs]() { return this->HuntWorker(threadIdx, maxEpochs); });

	for (auto& worker : workers) worker.join();
}

void SorterHunter::HuntWorker(size_t threadIdx, size_t maxEpochs)
{
	Network& networkCore = networkCores[threadIdx];

	// Load the provided initial network
	if (Config::HasKey("InitialNetwork"))
		networkCore = Config::GetNetwork("InitialNetwork");

	// Build an initial solution naively
	ProduceInitialSolution(networkCore);
	RegisterValidCore(networkCore);

	// Program never ends, keep trying to improve, we may restart in the outer loop however
	if (Config::Verbosity() >= VerbosityHigh) PRINT("{:<50}\n", "Hunting...");
	for (size_t epoch = 0;!maxEpochs || epoch < maxEpochs; epoch++)
	{
		if (Config::Verbosity() >= VerbosityDebug) LogEpoch(threadIdx, epoch);
		//if ((epoch + 1) % 1000 == 0) networkCore = convexHull.GetSmallestNetwork();

		// Mutate network
		Network mutatedCore{ networkCore };
		mutator.MutateMulti(mutatedCore, maxMutations);

		// Symmetrically expand new network and add postfix
		Network mutatedWithPostfix = symmetric ? symmetricExpansion(N, mutatedCore) : mutatedCore;
		mutatedWithPostfix = Concatenate(mutatedWithPostfix, postfix);

		// Test if this network is a valid sorter
		if (TestNetworkAndBump(mutatedWithPostfix))
		{
			networkCore = mutatedCore;
			RegisterValidCore(networkCore);
		}

		// With low probability, add another random pair at a random place to attempt to escape from local minima
		if (escapeRate > 0 && (GlobalGen()() % escapeRate) == 0)
			UphillStep(networkCore);
	}
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

void SorterHunter::ProduceInitialSolution(Network& networkCore)
{
	if (Config::Verbosity() >= VerbosityHigh) PRINT("{:<50}\r", "Producing initial solution...");

	// Produce initial solution, simply by adding random pairs until we found a valid network. In case no postfix is present, we demand that the added pair
	// fixes at least one of the output inversions in the first detected error output vector, so it does at least some useful work to help sorting the outputs.
	// In case there is a postfix network, this check is not implemented.
	for (;;)
	{
		// Symmetrically expand the network and concatenate with the postfix
		Network se = symmetric ? symmetricExpansion(N, networkCore) : networkCore;
		se = Concatenate(se, postfix);

		// Test the network
		auto [networkIsValid, problemInput] = TestNetwork(se);
		if (networkIsValid) break;

		if (!postfix.empty()) networkCore.push_back(RandomElement(alphabet));
		else
		{
			CE newCE;
			bool foundNewCE = false;
			do
			{
				newCE = RandomElement(alphabet);

				if ((((problemInput >> newCE.lo) & 1) == 1) && (((problemInput >> newCE.hi) & 1) == 0))
					foundNewCE = true;

				if (symmetric)
					if ((((problemInput >> ((N - 1) - newCE.hi)) & 1) == 1) && (((problemInput >> ((N - 1) - newCE.lo)) & 1) == 0))
						foundNewCE = true;

			} while (!foundNewCE);

			networkCore.push_back(newCE);
		}
	}
}

void SorterHunter::RunNetwork(const Network& network, BPWord* inputs)
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

std::pair<bool, SortWord> SorterHunter::TestNetwork(const Network& network)
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
			size_t bitIdx = CountrZero(accum);
			
			// Convert failing output into serial word
			SortWord problemOutput = 0;
			for (uint8_t inputIdx = 0; inputIdx < N; inputIdx++)
			{
				uint64_t bit = ReadBit(currentGroup[inputIdx], bitIdx);
				problemOutput |= bit << inputIdx;
			}

			return { false, problemOutput };
		}
	}

	return { true, 0 };
}

bool SorterHunter::TestNetworkAndBump(const Network& network)
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
			//size_t bitIdx = CountrZero(accum);
			//BumpVectorPosition(groupIdx, bitIdx);
			return false;
		}
	}

	return true;
}

void SorterHunter::BumpVectorPosition(size_t groupIdx, size_t bitIdx)
{
	/*
	if (groupIdx > 1)
	{
		// Move up failing vector group about 1/8 the distance to the front
		size_t groupStartIdx = N * groupIdx;
		size_t delta = N * ((groupIdx + 7) / 8);
		for (size_t k = 0; k < N; k++)
		{
			BPWord z = testVectors[groupStartIdx + k - delta];
			testVectors[groupStartIdx + k - delta] = testVectors[groupStartIdx + k];
			testVectors[groupStartIdx + k] = z;
		}
	}
	else if (groupIdx == 1)
	{
		// Swap with last bit position of group 0
		BPWord m0 = 1ull << (PARWORDSIZE - 1);
		BPWord m1 = 1ull << bitIdx;
		int shift = (PARWORDSIZE - 1) - bitIdx;
		for (size_t k = 0; k < N; k++)
		{
			BPWord old0 = testVectors[k];
			BPWord old1 = testVectors[k + N];
			testVectors[k] = (old0 & ~m0) | ((old1 & m1) << shift);
			testVectors[k + N] = (old1 & ~m1) | ((old0 & m0) >> shift);
		}
	}
	else if (bitIdx > 0) // groupno==0, bit position >0
	{
		// Swap with neighbouring bit position within group 0
		BPWord m0 = 1ull << (bitIdx - 1);
		BPWord m1 = 1ull << bitIdx;
		for (size_t k = 0; k < N; k++)
		{
			BPWord old = testVectors[k];
			testVectors[k] = (old & ~m0 & ~m1) | ((old & m1) >> 1) | ((old & m0) << 1);
		}
	}
	*/
}

void SorterHunter::LogEpoch(size_t threadIdx, size_t epoch)
{
	//if (epoch % 10000 == 0) PRINT("Epoch: {}\r", epoch);
}

void SorterHunter::RegisterValidCore(Network& networkCore)
{
	// Concatenate with prefix and postfix and calculate properties
	Network totalNetwork = symmetric ? symmetricExpansion(N, networkCore) : networkCore;
	totalNetwork = Concatenate(prefix, Concatenate(totalNetwork, postfix));

	// Check if this network improves the convex hull
	if (!convexHull.AddEntry(totalNetwork, networkCore)) return;

	// Reduce spam output by excluding neworks worse than bubblesort
	bool betterThanBubble = (totalNetwork.size() <= ((N * (N - 1)) / 2));
	if (!betterThanBubble && Config::Verbosity() < VerbosityHigh) return;

	// Print metadata and network
	//PrintNetwork(totalNetwork);
	convexHull.Print();
}

void SorterHunter::UphillStep(Network& networkCore)
{
	// Choose random insertion position and new CE
	size_t insertPos = GlobalGen()() % (networkCore.size() + 1);
	CE newCE = RandomElement(alphabet);

	// If not forcing a valid step, just add the pair straight away
	if (!Config::GetInt("ForceValidUphillStep", 1))
	{
		networkCore.insert(networkCore.begin() + insertPos, newCE);
		return;
	}

	// Determine if the new pair is within the last layer
	bool inLastLayer = true;
	for (size_t ceIdx = insertPos; ceIdx < networkCore.size(); ceIdx++)
	{
		auto [lo, hi] = networkCore[ceIdx];
		if ((lo == newCE.lo) || (hi == newCE.lo) || (lo == newCE.hi) || (hi == newCE.hi))
		{
			inLastLayer = false;
			break;
		}
	}

	// If in the last layer, add this pair
	// Otherwise, add a duplicate of the pair already at this position
	if (inLastLayer) networkCore.insert(networkCore.begin() + insertPos, newCE);
	else networkCore.insert(networkCore.begin() + insertPos, networkCore[insertPos]);
}
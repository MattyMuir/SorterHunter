#include "SimpleGeneticHunter.h"

#include <bit>
#include <thread>
#include <algorithm>

#include "Utility/version.h"
#include "Utility/print.h"
#include "Config/Config.h"
#include "Prefix/prefix_processor.h"
#include "Utility/GlobalRandom.h"

SimpleGeneticHunter::SimpleGeneticHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_)
	: SorterHunter(prefixGenerator_, postfix_, alphabet_),
	maxMutations(Config::GetInt("MaxMutations", 1)),
	escapeRate(Config::GetInt("EscapeRate", 0)),
	restartRate(Config::GetInt("RestartRate", 0)),
	mutator(alphabet_) {}

void SimpleGeneticHunter::StartHunting(size_t maxEpochs)
{
	// Generate a prefix
	prefix = prefixGenerator.Generate();

	// Prepare the test vectors based on the prefix
	PrepareTestVectors();

	// === Launch worker threads ===
	size_t numThreads = std::thread::hardware_concurrency() - 1;
	networkCores.resize(numThreads, Network{});

	workers.reserve(numThreads);
	for (size_t threadIdx = 0; threadIdx < numThreads; threadIdx++)
		workers.emplace_back([this, threadIdx, maxEpochs]() { return this->HuntWorker(threadIdx, maxEpochs); });
}

void SimpleGeneticHunter::StopHunting()
{
	shouldStop = true;
	for (auto& worker : workers) worker.join();
}

void SimpleGeneticHunter::HuntWorker(size_t threadIdx, size_t maxEpochs)
{
	Network& networkCore = networkCores[threadIdx];

	// Load the provided initial network
	if (Config::HasKey("InitialNetwork"))
		networkCore = SanitizeNetwork(Config::GetNetwork("InitialNetwork"));

	// Build an initial solution naively
	ProduceInitialSolution(networkCore);
	RegisterValidCore(networkCore);

	// Program never ends, keep trying to improve, we may restart in the outer loop however
	if (Config::Verbosity() >= VerbosityHigh) PRINT("{:<50}\n", "Hunting...");
	for (size_t epoch = 0;!maxEpochs || epoch < maxEpochs; epoch++)
	{
		if (shouldStop) break;
		if (Config::Verbosity() >= VerbosityDebug) LogEpoch(threadIdx, epoch);

		// Mutate network
		Network mutatedCore{ networkCore };
		mutator.MutateMulti(mutatedCore, maxMutations);

		// Symmetrically expand new network and add postfix
		Network mutatedWithPostfix = symmetric ? SymmetricExpansion(mutatedCore) : mutatedCore;
		mutatedWithPostfix = Concatenate(mutatedWithPostfix, postfix);

		// Test if this network is a valid sorter
		if (TestNetwork(mutatedWithPostfix))
		{
			networkCore = mutatedCore;
			RegisterValidCore(networkCore);
		}

		// With low probability, add another random pair at a random place to attempt to escape from local minima
		if (escapeRate > 0 && (GlobalGen()() % escapeRate) == 0)
			UphillStep(networkCore);
	}
}

void SimpleGeneticHunter::ProduceInitialSolution(Network& networkCore)
{
	if (Config::Verbosity() >= VerbosityHigh) PRINT("{:<50}\r", "Producing initial solution...");

	// Produce initial solution, simply by adding random pairs until we found a valid network. In case no postfix is present, we demand that the added pair
	// fixes at least one of the output inversions in the first detected error output vector, so it does at least some useful work to help sorting the outputs.
	// In case there is a postfix network, this check is not implemented.
	for (;;)
	{
		// Symmetrically expand the network and concatenate with the postfix
		Network se = symmetric ? SymmetricExpansion(networkCore) : networkCore;
		se = Concatenate(se, postfix);

		// Test the network
		SortWord problemOutput;
		bool networkIsValid = TestNetwork(se, &problemOutput);
		if (networkIsValid) break;

		if (!postfix.empty()) networkCore.push_back(RandomElement(alphabet));
		else
		{
			CE newCE;
			bool foundNewCE = false;
			do
			{
				newCE = RandomElement(alphabet);

				if ((((problemOutput >> newCE.lo) & 1) == 1) && (((problemOutput >> newCE.hi) & 1) == 0))
					foundNewCE = true;

				if (symmetric)
					if ((((problemOutput >> ((N - 1) - newCE.hi)) & 1) == 1) && (((problemOutput >> ((N - 1) - newCE.lo)) & 1) == 0))
						foundNewCE = true;

			} while (!foundNewCE);

			networkCore.push_back(newCE);
		}
	}
}

void SimpleGeneticHunter::LogEpoch(size_t threadIdx, size_t epoch)
{
	//if (epoch % 10000 == 0) PRINT("Epoch: {}\r", epoch);
}

void SimpleGeneticHunter::UphillStep(Network& networkCore)
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
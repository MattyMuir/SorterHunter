#include "PrefixGenerator.h"

#include "print.h"
#include "Config.h"
#include "prefix_processor.h"

PrefixGenerator::PrefixGenerator(PrefixType type_, const std::vector<CE>& alphabet_)
	: type(type_),
	alphabet(alphabet_),
	N(Config::GetInt("Ninputs")),
	symmetric(Config::GetInt("Symmetric", 0)),
	greedyPrefixSize(Config::GetInt("GreedyPrefixSize", 0))
{}

Network PrefixGenerator::Generate() const
{
	if (Config::Verbosity() >= VerbosityModerate)
		PRINT("{:<50}\r", "Generating prefix...");

	switch (type)
	{
	case PrefixNone: return {};
	case PrefixFixed: return Config::GetNetwork("FixedPrefix");
	case PrefixGreedy:
	{
		Network prefix;
		SortWord sizetmp = createGreedyPrefix(N, greedyPrefixSize, symmetric, alphabet, prefix);
		if (Config::Verbosity() >= VerbosityHigh)
			PRINT("Greedy prefix size {}, span {}.\n", prefix.size(), sizetmp);
		return prefix;
	}
	case PrefixHybrid:
	{
		Network prefix = Config::GetNetwork("FixedPrefix");
		SortWord sizetmp = createGreedyPrefix(N, greedyPrefixSize + prefix.size(), symmetric, alphabet, prefix);
		if (Config::Verbosity() >= VerbosityHigh)
			PRINT("Hybrid prefix size {}, span {}.\n", prefix.size(), (size_t)sizetmp);
		return prefix;
	}
	default: PRINT("Invalid prefix type!");
	}
	return {};
}
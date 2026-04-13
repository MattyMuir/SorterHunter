#include "version.h"
#include "print.h"
#include "Config.h"
#include "GlobalRandom.h"
#include "SorterHunter.h"
#include "PrefixGenerator.h"
#include "Timer.h"

int PrintUsage()
{
	PRINT("Usage: SorterHunter <config_file_name>\n\n"
	"A sample config file containing help text is provided, named 'sample_config.txt'\n"
	"SorterHunter is a program that tries to find efficient sorting networks by applying\n"
	"an evolutionary approach. It is offered under MIT license\n"
	"Program version: {}\n", VERSION);
	return -1;
}

bool ProcessConfig(const std::string& filepath)
{
	try { Config::Parse(filepath); }
	catch (const ParseError& err)
	{
		PRINT("{}", err.what());
		return false;
	}
	return true;
}

std::vector<CE> BuildAlphabet()
{
	std::vector<CE> alphabet;
	uint8_t N = Config::GetInt("Ninputs");
	bool symmetric = Config::GetInt("Symmetric", 0);

	for (uint32_t i = 0; i < (N - 1u); i++)
	{
		for (uint32_t j = i + 1; j < N; j++)
		{
			uint32_t isym = N - 1 - j;
			uint32_t jsym = N - 1 - i;

			if (!symmetric || (isym > i) || ((isym == i) && (jsym >= j)))
				alphabet.push_back({ (uint8_t)i,(uint8_t)j });
		}
	}

	return alphabet;
}

int main(int argc, char** argv)
{
	// Process config file
	if (argc != 2) return PrintUsage();
	if (!ProcessConfig(argv[1])) return -1;

	// Generate alphabet
	std::vector<CE> alphabet = BuildAlphabet();

	// Hunt for efficient sorting networks
	PrefixGenerator prefixGenerator{ (PrefixType)Config::GetInt("PrefixType", 0), alphabet };
	SorterHunter hunter{ prefixGenerator, Config::GetNetwork("Postfix", Network{}), alphabet };
	TIMER(t);
	hunter.Hunt();
	STOP_LOG(t);
}
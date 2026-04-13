#include "Config.h"

#include <cinttypes>
#include <fstream>
#include <ranges>
#include <format>
#include <charconv>

enum class KeyType { Int, Network };
static const std::unordered_map<std::string, KeyType> allKeys{
	{ "Ninputs",						KeyType::Int },
	{ "Symmetric",						KeyType::Int },
	{ "RandomSeed",						KeyType::Int },
	{ "EscapeRate",						KeyType::Int },
	{ "ForceValidUphillStep",			KeyType::Int },
	{ "MaxMutations",					KeyType::Int },
	{ "WeightRemovePair",				KeyType::Int },
	{ "WeightSwapPairs",				KeyType::Int },
	{ "WeightReplacePair",				KeyType::Int },
	{ "WeightCrossPairs",				KeyType::Int },
	{ "WeightSwapIntersectingPairs",	KeyType::Int },
	{ "WeightReplaceHalfPair",			KeyType::Int },
	{ "PrefixType",						KeyType::Int },
	{ "GreedyPrefixSize",				KeyType::Int },
	{ "RestartRate",					KeyType::Int },
	{ "Verbosity",						KeyType::Int },

	{ "FixedPrefix",					KeyType::Network },
	{ "Postfix",						KeyType::Network },
	{ "InitialNetwork",					KeyType::Network },
};

static std::unordered_map<std::string, std::variant<uint64_t, Network>> keyMap;

ParseError::ParseError(const std::string& msg)
	: std::runtime_error(msg) {}

static void StripLine(std::string& line)
{
	// Remove comments
	line = line.substr(0, line.find('#'));

	// Remove whitespace
	std::erase_if(line, [](char c) { return (bool)std::isspace(c); });
}

static std::vector<std::string> Split(std::string_view str, char delim)
{
	std::vector<std::string> parts;
	for (const auto& part : std::views::split(str, delim))
		parts.emplace_back(part.begin(), part.end());
	return parts;
}

void Config::Parse(const std::string& filepath)
{
	// Open file
	std::ifstream file{ filepath };
	if (!file) throw ParseError{ "Could not open config file" };
	
	std::string line;
	while (std::getline(file, line))
	{
		// Strip comments and whitespace
		StripLine(line);
		if (line.empty()) continue;

		// Split line by '=' character
		auto parts = Split(line, '=');
		if (parts.size() != 2)
			throw ParseError{ std::format("Expected a single = per line, found {}", parts.size() - 1)};

		AddKeyValue(parts[0], parts[1]);
	}
}

bool Config::HasKey(const std::string& key)
{
	return keyMap.contains(key);
}

uint64_t Config::GetInt(const std::string& key, std::optional<uint64_t> def)
{
	if (keyMap.contains(key)) return std::get<uint64_t>(keyMap.at(key));
	if (def.has_value()) return def.value();
	throw ParseError{ std::format("Config missing required key '{}'", key) };
}

const Network& Config::GetNetwork(const std::string& key, const std::optional<Network>& def)
{
	if (keyMap.contains(key)) return std::get<Network>(keyMap.at(key));
	if (def.has_value()) return def.value();
	throw ParseError{ std::format("Config missing required key '{}'", key) };
}

Verbosity Config::Verbosity()
{
	return (::Verbosity)GetInt("Verbosity", VerbosityMinimal);
}

void Config::AddKeyValue(const std::string& key, const std::string& valueStr)
{
	if (!allKeys.contains(key))
		throw ParseError{ std::format("Invalid config key '{}'", key) };

	if (keyMap.contains(key))
		throw ParseError{ std::format("Duplicate config key '{}'", key) };

	switch (allKeys.at(key))
	{
	case KeyType::Int:
		keyMap[key] = ParseInt(valueStr);
		break;
	case KeyType::Network:
		keyMap[key] = ParseNetwork(valueStr);
		break;
	}
}

uint64_t Config::ParseInt(const std::string& valueStr)
{
	uint64_t value;
	auto [ptr, errc] = std::from_chars(valueStr.data(), valueStr.data() + valueStr.size(), value);
	if (errc != std::errc{})
		throw ParseError{ std::format("Expected integer, got '{}'", valueStr) };
	return value;
}

Network Config::ParseNetwork(const std::string& valueStr)
{
	Network network;
	uint8_t lo, hi;
	const char* it = valueStr.c_str();
	const char* strEnd = it + valueStr.size();
	while (it < strEnd)
	{
		if (sscanf_s(it, "(%" SCNu8 ",%" SCNu8 ")", &lo, &hi) != 2)
			throw ParseError{ "Invalid network!" };
		network.push_back({ lo, hi });
		it = strchr(it, ')') + 2;
	}
	return network;
}
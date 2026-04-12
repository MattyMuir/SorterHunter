#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <variant>

#include "types.h"
#include "Network.h"

enum Verbosity { VerbosityMinimal, VerbosityModerate, VerbosityHigh, VerbosityDebug };

class ParseError : public std::runtime_error
{
public:
	ParseError(const std::string& msg);
};

class Config
{
public:
	Config() = delete;

	static void Parse(const std::string& filepath);

	static bool HasKey(const std::string& key);
	static uint64_t GetInt(const std::string& key, std::optional<uint64_t> def = std::nullopt);
	static const Network& GetNetwork(const std::string& key, const std::optional<Network>& def = std::nullopt);
	static Verbosity Verbosity();

protected:
	static void AddKeyValue(const std::string& key, const std::string& valueStr);
	static uint64_t ParseInt(const std::string& valueStr);
	static Network ParseNetwork(const std::string& valueStr);
};
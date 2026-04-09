#pragma once
#include <string>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <variant>

#include "types.h"

class ParseError : public std::runtime_error
{
public:
	ParseError(const std::string& msg);
};

class ConfigParser
{
public:
	void Parse(const std::string& filepath);

	bool HasKey(const std::string& key) const;
	uint64_t GetInt(const std::string& key, std::optional<uint64_t> def = std::nullopt) const;
	const Network& GetNetwork(const std::string& key) const;

protected:
	std::unordered_map<std::string, std::variant<uint64_t, Network>> keyMap;

	void AddKeyValue(const std::string& key, const std::string& valueStr);
	static uint64_t ParseInt(const std::string& valueStr);
	static Network ParseNetwork(const std::string& valueStr);
};
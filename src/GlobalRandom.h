#pragma once
#include <random>

std::mt19937_64& GlobalGen();

template <typename Ty>
size_t RandomIndex(const std::vector<Ty>& vec)
{
	std::uniform_int_distribution<size_t> dist{ 0, vec.size() - 1 };
	return dist(GlobalGen());
}

template <typename Ty>
const Ty& RandomElement(const std::vector<Ty>& vec)
{
	return vec[RandomIndex(vec)];
}

template <typename Ty>
Ty& RandomElement(std::vector<Ty>& vec)
{
	return vec[RandomIndex(vec)];
}
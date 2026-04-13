#include "GlobalRandom.h"

#include "Config.h"

std::atomic<size_t> threadIdx = 0;

static inline uint64_t SplitMix(uint64_t x)
{
	x += 0x9e3779b97f4a7c15ULL;
	x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
	x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
	return x ^ (x >> 31);
}

std::mt19937_64 InitializeGenerator()
{
	size_t thisThreadIdx = threadIdx++;
	uint64_t baseSeed = Config::GetInt("RandomSeed", std::random_device{}());
	return std::mt19937_64{ SplitMix(baseSeed ^ (thisThreadIdx * 0x9e3779b97f4a7c15ULL)) };
}

std::mt19937_64& GlobalGen()
{
	thread_local std::mt19937_64 gen = InitializeGenerator();
	return gen;
}
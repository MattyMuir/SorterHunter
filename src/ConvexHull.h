#pragma once
#include <utility>
#include <vector>
#include <mutex>

#include "Network.h"

class ConvexHull
{
protected:
	struct Entry
	{
		Network network;
		uint32_t size, depth;
	};

public:
	void Clear();
	bool AddEntry(const Network& network);
	void Print() const;

protected:
	std::mutex mu;
	std::vector<Entry> entries;

	static bool EntryDominates(Entry a, Entry b);
};
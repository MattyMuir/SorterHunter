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
		Network totalNetwork, networkCore;
		uint32_t size, depth;
	};

public:
	void Clear();
	bool AddEntry(const Network& totalNetwork, const Network& networkCore);
	void Print() const;
	Network GetSmallestNetwork() const;

protected:
	mutable std::mutex mu;
	std::vector<Entry> entries;

	static bool EntryDominates(Entry a, Entry b);
};
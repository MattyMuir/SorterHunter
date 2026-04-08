#pragma once
#include <utility>
#include <vector>

class ConvexHull
{
protected:
	struct Entry { uint32_t size, depth; };

public:
	void Clear();
	bool AddEntry(uint32_t size, uint32_t depth);
	void Print() const;

protected:
	std::vector<Entry> entries;

	static bool EntryDominates(Entry a, Entry b);
};
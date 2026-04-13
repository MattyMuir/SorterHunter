#include "ConvexHull.h"

#include "print.h"

void ConvexHull::Clear()
{
	std::unique_lock lock{ mu };
	entries.clear();
}

bool ConvexHull::AddEntry(const Network& network)
{
	std::unique_lock lock{ mu };
	Entry newEntry{ network, (uint32_t)network.size(), ComputeDepth(network) };

	// Check if an existing entry already dominates this one
	for (const Entry& existingEntry : entries)
		if (EntryDominates(existingEntry, newEntry)) return false;

	// If not, remove all entries that this one dominates
	std::erase_if(entries, [newEntry](const Entry& e) { return EntryDominates(newEntry, e); });

	// Add the new entry
	entries.push_back(newEntry);
	return true;
}

void ConvexHull::Print() const
{
	PRINT("Most performant: [");
	for (size_t i = 0; i < entries.size(); i++)
	{
		if (i) PRINT(", ");
		PRINT("({},{})", entries[i].size, entries[i].depth);
	}
	PRINT("]\n");
}

bool ConvexHull::EntryDominates(Entry a, Entry b)
{
	return (a.size <= b.size) && (a.depth <= b.depth);
}
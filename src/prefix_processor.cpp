/**
 * @file prefix_processor.cpp
 * @brief Network prefix related operations for SorterHunter program
 * @author Bert Dobbelaere bert.o.dobbelaere[at]telenet[dot]be
 *
 * Copyright (c) 2022 Bert Dobbelaere
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <algorithm>
#include <cstdio>
#include <cassert>

#include "hutils.h"
#include "prefix_processor.h"
#include "print.h"

extern uint32_t Verbosity;

/**
 * Replaces a *sorted* list of patterns applied to a network containing a single CE by the sorted list of output patterns of that network.
 * The sort order is low to high, a pattern represents the binary representation of an input/output state
 * Restriction to sorted pattern lists allows to compute the output list in linear time.
 * @param patterns [IN/OUT] Input and output list of patterns, sorted.
 * @param pair Representation of CE to apply
 */
static void swap_sortedpatterns( SinglePatternList &patterns, const CE &pair)
{
	SortWord p=1ULL << pair.lo;
	SortWord q=1ULL << pair.hi;
	SortWord mask=p|q;
	
	SinglePatternList res;
	
	size_t idxp=0;
	size_t idxnp=0;
	size_t l=patterns.size();
	SortWord last=-1;
	
	while ((idxp<l) &&((patterns[idxp] &mask)!=p)) { idxp++; }
	while ((idxnp<l)&&((patterns[idxnp]&mask)==p)) { idxnp++; }
	
	while ((idxnp<l)&&(idxp<l))
	{
		SortWord a=patterns[idxp]^mask;
		SortWord b=patterns[idxnp];
		if (a<b)
		{
			if (a!=last)
			{
				res.push_back(a);
				last=a;
			}
			idxp++;
			while ((idxp<l)&&((patterns[idxp]&mask)!=p)) { idxp++; }
		}
		else
		{
			if (a!=last)
			{
				res.push_back(b);
				last=b;
			}
			idxnp++;
			while ((idxnp<l)&&((patterns[idxnp]&mask)==p)) { idxnp++; }
		}
	}
	while (idxnp<l)
	{
		res.push_back(patterns[idxnp++]);
	}
	while (idxp<l)
	{
		res.push_back(patterns[idxp++]^mask);
	}
	
	patterns=res;
}

/**
 * Helper class to efficiently compute partially ordered pattern sets.
 * The inputs of the network are grouped together in clusters that have been connected by CEs
 * Initial clusters contain just one input (no CEs added yet). 
 * Each cluster has a set of output patterns that it leaves behind. The global set of output patterns
 * is at each time defined by the bitwise "ored" combinations of the outputs that all clusters produce together.
 * While adding CEs to the network, clusters are combined into larger clusters with a shrinking total number of
 * output patterns. If the CE is added that combines the last two clusters, only one cluster will remain.
 * If after that sufficient new CEs are added, only ninputs+1 patterns will remain, meaning that the network is fully sorted.
 */
class ClusterGroup{
	public:
		ClusterGroup(uint8_t ninputs);
		ClusterGroup(const ClusterGroup &cg);
		const ClusterGroup& operator=(const ClusterGroup &cg);
		void clear();
		void preSort(CE p);
		void computeOutputs(SinglePatternList &patterns) const;
		SortWord outputSize() const;
		bool isSameCluster(CE p) const;
		~ClusterGroup();
	private:
		void combine(uint8_t ci_idx, uint8_t cj_idx);
		
		SinglePatternList *patternlists; ///< Sorted list of output patterns from each cluster of lines
		SortWord *masks; ///< Masks for each cluster marking the applicable lines for each cluster
		uint8_t *clusterAlloc; ///< Allocations of lines to clusters
		uint8_t ninputs; ///< Total number of inputs (and outputs) of the network
};


/**
 * Initialize an empty cluster group
 */
ClusterGroup::ClusterGroup(uint8_t n)
{
	ninputs=n;
	patternlists = new SinglePatternList[ninputs];
	masks = new SortWord[ninputs];
	clusterAlloc = new uint8_t[ninputs];
	clear();
}

/**
 * Copy constructor
 */
ClusterGroup::ClusterGroup(const ClusterGroup &cg)
{
	ninputs = cg.ninputs;
	patternlists = new SinglePatternList[ninputs];
	masks = new SortWord[ninputs];
	clusterAlloc = new uint8_t[ninputs];
	for (uint32_t k=0;k<ninputs;k++)
	{
		patternlists[k] = cg.patternlists[k];
		masks[k]=cg.masks[k];
		clusterAlloc[k]=cg.clusterAlloc[k];
	}
}

/**
 * Assignment of cluster groups to eachother
 */
const ClusterGroup& ClusterGroup::operator=(const ClusterGroup &cg)
{
	ninputs = cg.ninputs;
	for (uint32_t k=0;k<ninputs;k++)
	{
		patternlists[k] = cg.patternlists[k];
		masks[k]=cg.masks[k];
		clusterAlloc[k]=cg.clusterAlloc[k];
	}	
	return *this;
}

/**
 * Clean up cluster group
 */
ClusterGroup::~ClusterGroup()
{
	delete[] patternlists;
	delete[] masks;
	delete[] clusterAlloc;
}

/**
 * Set initial state:
 * each input corresponds one to one with its own cluster. The cluster has two possible output patterns:
 * the all 0 pattern, and a single 1 bit at the bit position of the corresponding input.
 */
void ClusterGroup::clear()
{
	for (uint32_t k=0;k<ninputs;k++)
	{
		clusterAlloc[k]=k;
		masks[k]=1ULL<<k;
		patternlists[k].clear();
		patternlists[k].push_back(0);
		patternlists[k].push_back(1ULL<<k);
	}	
}

/**
 * Combines two clusters to form a larger cluster.
 * The output pattern list is produced by bitwise "oring" of both original pattern lists
 * @param ci_idx First cluster index (new result cluster)
 * @param cj_idx Second cluster index (will no longer be used)
 */
void ClusterGroup::combine(uint8_t ci_idx, uint8_t cj_idx)
{
	SinglePatternList &p1=patternlists[ci_idx];
	SinglePatternList &p2=patternlists[cj_idx];
	
	for (uint32_t k=0;k<ninputs;k++)
		if (clusterAlloc[k]==cj_idx)
			clusterAlloc[k]=ci_idx; // ci will take over
	
	masks[ci_idx]|=masks[cj_idx];
	SinglePatternList cp;
	/*
	 * Combined cluster's output patterns are here simply generated by producing all
	 * patterns, first disregarding their final order and sorting them afterwards.
	 * At first, I had an algorithm in place that broke the masks into chunks allowing
	 * in order generation that had lower theoretical complexity. For practical sizes however
	 * a quicksort proved a faster and simpler alternative. (and probably has less bugs :-) )
	 */
	for (size_t i=0;i<p1.size();i++)
		for (size_t j=0;j<p2.size();j++)
			cp.push_back( p1[i] | p2[j]);
	std::sort(cp.begin(),cp.end()); // Keep the new output set sorted
	p1=cp;
	masks[cj_idx]=0;
	p2.clear();
}


bool ClusterGroup::isSameCluster(CE p) const
{
	uint32_t	ci_idx=clusterAlloc[p.lo];
	uint32_t cj_idx=clusterAlloc[p.hi];
	return ci_idx==cj_idx;
}

/**
 * Reduces the number of patterns represented by appending a single CE to the network.
 * If the CE's lines belong to different clusters, the clusters are merged first.
 * @param p CE represented by its input/output lines
 */
void ClusterGroup::preSort(CE p)
{
	uint32_t	ci_idx=clusterAlloc[p.lo];
	uint32_t cj_idx=clusterAlloc[p.hi];
	
	if (ci_idx!=cj_idx)
	{
		combine(ci_idx,cj_idx);
	}
	swap_sortedpatterns( patternlists[ci_idx], p);
}

/**
 * Compute the list of output patterns that can leave the network composed of all
 * clusters remaining. This is done by "oring" together output combinations of all remaining clusters.
 * @param patterns [OUT] pattern list created (not lexographically sorted)
 */
void ClusterGroup::computeOutputs(SinglePatternList &patterns) const
{
	const static SinglePatternList *pLists[NMAX];
	int n_to_combine=0;
	
	for (uint32_t k=0;k<ninputs;k++)
	{
		if (masks[k]!=0)
			pLists[n_to_combine++] = &patternlists[k];
	}
	
	assert(n_to_combine>0);
	
	int level=0;
	size_t indices[NMAX]={0};
	SortWord outmasks[NMAX]={0};
	patterns.clear();
	
	while (level>=0)
	{	
		if (indices[level]< pLists[level]->size())
		{
			if (level==0)
				outmasks[level] = (*pLists[level])[indices[level]];
			else
				outmasks[level] = outmasks[level-1] | (*pLists[level])[indices[level]];
			if (level<(n_to_combine-1))
			{
				indices[level+1]=0;
				indices[level]++;
				level++;
			}
			else
			{
				patterns.push_back( outmasks[level]);
				indices[level]++;
			}		
		}
		else
		{
			level--;
		}
	}
}

/**
 * Compute number of output patterns that would be produced by call to computeOutputs
 */
SortWord ClusterGroup::outputSize() const
{
	SortWord prod=1;
	
	for (uint32_t k=0;k<ninputs;k++)
	{
		if (masks[k]!=0)
			prod *= patternlists[k].size();
	}
	
#if 1
	if (prod==0) // Special case for N=NMAX, dirty hack avoiding wrap-around to 0 of empty network: set size to one less.
		prod-=1;
#endif
	
	return prod;
}


void computePrefixOutputs(uint8_t ninputs, const Network &prefix, SinglePatternList &patterns)
{
	ClusterGroup cg(ninputs);
	Network todo = prefix;
	
	while (!todo.empty())
	{
		cg.preSort(todo[0]); // Process first remaining pair, combine related clusters
		
		Network postponed;
		SortWord visitmask=0;
		for (size_t k=1;k<todo.size();k++) // Skip 1st element, we just handled it
		{
			CE el=todo[k];
			SortWord elmask = (1ull<<el.lo)|(1ull<<el.hi);
			
			if (((visitmask & elmask)==0) && cg.isSameCluster(el))
			{
				// Prioritize elements that can be applied without extra cluster joining.
				// The goal is to reduce memory requirements where possible
				cg.preSort(el);
			}
			else
			{
				// Postpone till next iteration any element that requires additional clusters to be joined or has dependencies to unprocessed elements
				postponed.push_back(el); 
			}
			visitmask |= elmask;
		}
		todo = postponed;
	}
	
	cg.computeOutputs(patterns);
}

/**
 * For symmetric networks, any network that sorts a pattern successfully will also sort the reverse of the inverse,
 * i.e. if a symmetric network sorts '00101111', if will also sort '00001011'
 * This function is used to discard the largest of those patterns.
 */
static bool hasSmallerMirror(uint8_t ninputs, SortWord w)
{
	SortWord rw=0u;
	SortWord tmp=w;
	for (uint32_t k=0;k<ninputs;k++)
	{
		rw <<= 1;
		rw |= ~tmp & 1u;
		tmp >>= 1;
	}
	return w > rw;
}


static SortWord all_n_inputs_mask; ///< ninputs lowest bit to be set

static bool isSorted(SortWord w)
{
	w = ~w & all_n_inputs_mask;
	return (w&(w+1)) == 0;
}


void convertToBitParallel(uint8_t ninputs, const SinglePatternList &singles, bool use_symmetry, BitParallelList &parallels)
{
	uint32_t level=0;
	static BPWord buffer[NMAX];
	parallels.clear();
	
	all_n_inputs_mask = 0ULL;
	for (uint32_t k=0;k<ninputs;k++)
	{
		all_n_inputs_mask |= 1ULL << k;
	}
	
	for (size_t idx=0;idx<singles.size();idx++)
	{
		SortWord w=singles[idx];
		if (use_symmetry && hasSmallerMirror(ninputs, w))
		{
			continue; // Complement of reverse word is smaller, skip this vector if the network is symmetric
		}
		
		if (isSorted(w))
		{
			continue; // Already sorted pattern will not be affected by sorting operation - useless as test vector
		}
		
		for (uint32_t b=0;b<ninputs;b++)
		{
			buffer[b]<<=1;
			buffer[b]|=(w&1);
			w>>=1;
		}
		level++;
		
		if (level>=PARWORDSIZE)
		{
			for (uint32_t b=0;b<ninputs;b++)
			{
				parallels.push_back(buffer[b]);
				buffer[b]=0; // Needed ? Probably not, but cleaner.
			}
			level=0;			
		}	
	}
	if (level>0)
	{
		for (uint32_t b=0;b<ninputs;b++)
		{
			parallels.push_back(buffer[b]);
		}
	}

	if (Verbosity > 2)
	{
		PRINT("Debug: Pattern conversion: {} single inputs -> {} parallel words ({} * {}) (symmetry:{})\n", singles.size(), parallels.size(), ninputs, parallels.size()/ninputs, use_symmetry);
	}
}

static Network alphabet; ///< "Alphabet" of possible CEs defined by their vertical positions.

/**
 * Initialize alphabet of CEs. 
 * @param ninputs Number of network inputs
 * @param use_symmetry If set to true duplicates due to mirroring will be omitted
 */
static void initAlphabet(uint8_t ninputs, bool use_symmetry)
{
	alphabet.clear();
	for (uint32_t i=0;i<(ninputs-1u);i++)
		for (uint32_t j=i+1;j<ninputs;j++)
		{
			uint8_t isym=ninputs-1-j;
			uint8_t jsym=ninputs-1-i;
			
			if (!use_symmetry || (isym>i) || ((isym==i) && (jsym>=j)))
			{
				CE p={(uint8_t)i,(uint8_t)j};
				alphabet.push_back(p);
			}	
		}
}		

SortWord createGreedyPrefix(uint8_t ninputs, uint32_t maxpairs, bool use_symmetry, Network &prefix, RandGen_t &rndgen)
{
	if (Verbosity>2)
		PRINT("Creating greedy prefix. Initial prefix size = {}, max prefix size {}.\n",prefix.size(),maxpairs);

	ClusterGroup cg(ninputs);
	initAlphabet(ninputs, use_symmetry);

	for (size_t k=0;k<prefix.size();k++)
		cg.preSort(prefix[k]);
	SortWord currentsize = cg.outputSize();
	
	while ((prefix.size() < maxpairs) || (use_symmetry && (prefix.size()<(maxpairs-1))))
	{
		Network ashuf = alphabet;
		CE best= {0,1};
		std::shuffle(ashuf.begin(),ashuf.end(), rndgen);
		SortWord minsize = currentsize;

		ClusterGroup cgbest=cg;
		SortWord minfuturesize = currentsize;
		for (size_t k=0;k<alphabet.size();k++)
		{
			ClusterGroup cgnew = cg;
			cgnew.preSort(ashuf[k]);
			if (use_symmetry && ((ashuf[k].lo+ashuf[k].hi) != (ninputs-1)))
			{
				CE p = { (uint8_t)(ninputs-1-ashuf[k].hi), (uint8_t)(ninputs-1-ashuf[k].lo) };
				cgnew.preSort(p);
			}
			SortWord newsize = cgnew.outputSize();
			SortWord futuresize = newsize;
			if (futuresize<minfuturesize)
			{
				minsize=newsize;
				minfuturesize=futuresize;
				best=ashuf[k];
				cgbest=cgnew;
			}
		}
		
		if (minsize>=currentsize)
		{
			// Found no improvement
			if (Verbosity>2)
			{
				PRINT("Greedy algorithm: no further improvement.\n");
			}
			break;
		}
		cg=cgbest;
		if (Verbosity>2)
		{
			PRINT("Greedy: adding pair ({},{})\n",best.lo,best.hi);
		}
		prefix.push_back(best);
		if (use_symmetry && ((best.lo+best.hi) != (ninputs-1)))
		{
			CE p = { (uint8_t)(ninputs-1-best.hi), (uint8_t)(ninputs-1-best.lo) };
			if (Verbosity>2)
			{
				PRINT("Greedy: adding symmetric pair ({},{})\n",p.lo,p.hi);
			}
			prefix.push_back(p);
		}		
		currentsize=minsize;
	}
	return currentsize;
}


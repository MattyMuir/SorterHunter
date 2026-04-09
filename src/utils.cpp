/**
 * @file hutils.cpp
 * @brief Various utility functions for SorterHunter program
 * @author Bert Dobbelaere bert.o.dobbelaere[at]telenet[dot]be
 *
 * Copyright (c) 2017 Bert Dobbelaere
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
#include "utils.h"

#include <cstdio>

#include "print.h"

uint32_t computeDepth(const Network &nw)
{
	std::vector<SortWord> layers;
	int nlayers=0;
	
	for (size_t k=0;k<nw.size();k++)
	{
		uint32_t i=nw[k].lo;
		uint32_t j=nw[k].hi;
		int matchidx=nlayers;
		int idx=nlayers-1;
		while (idx>=0)
		{
			if ((layers[idx] & ((1ULL<<i)|(1ULL<<j))) == 0)
			{
				matchidx=idx;
			}
			else
			{
				break;
			}
			idx--;
		}
		if (matchidx>=nlayers)
		{
			layers.push_back(0);
			nlayers++;
		}
		layers[matchidx]|= 1ULL<<i;
		layers[matchidx]|= 1ULL<<j;
	}
	
	return nlayers;
}

void printnw(const Network &nw)
{
	PRINT("[");
	for (size_t k=0;k<nw.size();k++)
	{
		PRINT("({},{})",nw[k].lo,nw[k].hi);
		PRINT("{}", ((k+1)<nw.size()) ? ',':']');
	}
	PRINT("}}\r\n");
}

void symmetricExpansion(uint8_t ninputs, const Network &inpairs, Network &outpairs)
{
	outpairs.clear();
	for (size_t k=0;k<inpairs.size();k++)
	{
		outpairs.push_back(inpairs[k]);
		if ((inpairs[k].lo+inpairs[k].hi)!=(ninputs-1)) // Don't duplicate pair that maps on itself
		{
			CE sp={(uint8_t)(ninputs-1-inpairs[k].hi), (uint8_t)(ninputs-1-inpairs[k].lo)};
			outpairs.push_back(sp);
		}
	}
}

void concatNetwork(const Network &nw1, const Network &nw2, Network &result)
{
	result=nw1;
	result.insert(result.end(),nw2.begin(),nw2.end());
}

void appendNetwork(Network &dst, const Network &src)
{
	if (!src.empty())
	{
		dst.insert(dst.end(),src.begin(),src.end());
	}
}
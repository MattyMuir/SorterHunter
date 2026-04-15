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

Network SymmetricExpansion(uint8_t ninputs, const Network &inpairs)
{
	Network outpairs;
	for (const CE& inpair : inpairs)
	{
		outpairs.push_back(inpair);
		if (inpair.lo + inpair.hi == ninputs - 1) continue;

		CE sp = { (uint8_t)(ninputs - 1 - inpair.hi), (uint8_t)(ninputs - 1 - inpair.lo) };
		outpairs.push_back(sp);
	}

	return outpairs;
}
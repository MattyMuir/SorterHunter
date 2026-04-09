#pragma once
/**
 * @file utils.h
 * @brief Various utility functions for SorterHunter program
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

#include "types.h"
#include <random>

inline uint32_t min(uint32_t x,uint32_t y) { return (x<y)?x:y;} ///< Classic minimum
inline uint32_t max(uint32_t x,uint32_t y) { return (x>y)?x:y;} ///< Classic maximum

/**
 * Compute the number of layers in a sorting network
 * @param nw Input network
 * @return Number of parallel operation layers
 */
uint32_t computeDepth(const Network &nw);

/**
 * Create "symmetric" sorting network by creating a mirror image of each pair if it doesn't coincide with the original.
 * @param ninputs Number of inputs
 * @param inpairs Input network
 * @param outpairs Symmetrical output network 
 */
void symmetricExpansion(uint8_t ninputs, const Network &inpairs, Network &outpairs);

/**
 * Print a sorting network as text
 * @param nw network to print
 */
void printnw(const Network &nw);

/**
 * Concatenate two (partial) sorting networks into a new one
 * @param nw1 First network
 * @param nw2 Second network
 * @param result [OUT] Concatenation of both networks
 */
void concatNetwork(const Network &nw1, const Network &nw2, Network &result);


/**
 * Append one network to another
 * @param dst Network to be modified
 * @param src Network to be appended
 */
void appendNetwork(Network &dst, const Network &src);

// Random generation defs

typedef std::mt19937_64 RandGen_t;

#define RANDIDX(v) (mtRand()%v.size())      ///< Random index from vector
#define RANDELEM(v) (v[RANDIDX(v)])         ///< Random element from vector
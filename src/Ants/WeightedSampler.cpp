#include "WeightedSampler.h"

#include <bit>

WeightedSampler::WeightedSampler(size_t n_)
	: n(n_), size(std::bit_ceil(n_)), tree(size * 2)
{
    for (size_t idx = 0; idx < n; idx++)
        Update(idx, 1.0f);
}

void WeightedSampler::Update(size_t idx, float newWeight)
{
	size_t leafIdx = size + idx;
	tree[leafIdx] = newWeight;
	for (size_t i = leafIdx / 2; i >= 1; i /= 2)
		tree[i] = tree[2 * i] + tree[2 * i + 1];
}

float WeightedSampler::Read(size_t idx) const
{
	return tree[size + idx];
}

size_t WeightedSampler::Sample(std::mt19937_64& gen) const
{
    std::uniform_real_distribution<float> dist{ 0.0f, TotalWeight() };
    float r = dist(gen);

    size_t i = 1;
    while (i < size)
    {
        size_t left = 2 * i;
        if (r < tree[left])
            i = left;
        else
        {
            r -= tree[left];
            i = left + 1;
        }
    }

    // Guard against fp edge cases landing in the zero-padded region
    size_t result = i - size;
    return (result < n) ? result : n - 1;
}

void WeightedSampler::Scale(float scale)
{
    for (float& weight : tree) weight *= scale;
}

bool WeightedSampler::IsEmpty() const
{
	return n == 0;
}

float WeightedSampler::TotalWeight() const
{
	return IsEmpty() ? 0.0f : tree[1];
}
#pragma once
#include <random>

class WeightedSampler
{
public:
    WeightedSampler(size_t n_);

    void Update(size_t idx, float newWeight);
    float Read(size_t idx) const;
    size_t Sample(std::mt19937_64& gen) const;
    void Scale(float scale);

    bool IsEmpty() const;
    float TotalWeight() const;

private:
    size_t n, size;
    std::vector<float> tree;
};
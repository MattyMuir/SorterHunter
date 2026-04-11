#pragma once
#include <random>

inline std::mt19937_64 GlobalGen{ std::random_device{}() };
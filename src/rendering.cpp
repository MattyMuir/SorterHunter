#include "rendering.h"

#include <algorithm>
#include <format>

#include <raylib.h>

std::vector<Network> SplitByLayers(const Network& network)
{
	std::vector<uint64_t> usedChannels;
	std::vector<Network> layers;

	for (CE ce : network)
	{
		auto [lo, hi] = ce;
		uint64_t ceChannels = (1ULL << lo) | (1ULL << hi);
		int insertLayer;
		for (insertLayer = layers.size() - 1; insertLayer >= 0; insertLayer--)
		{
			if (usedChannels[insertLayer] & ceChannels) break;
		}
		insertLayer++;

		if (insertLayer >= layers.size())
		{
			usedChannels.push_back(0);
			layers.push_back({});
		}

		usedChannels[insertLayer] |= ceChannels;
		layers[insertLayer].push_back(ce);
	}

	return layers;
}

bool Overlaps(CE a, CE b)
{
	bool noOverlap = (a.hi < b.lo) || (b.hi < a.lo);
	return !noOverlap;
}

bool FitsInSublayer(const Network& sublayer, CE ce)
{
	return !std::any_of(sublayer.begin(), sublayer.end(), [ce](CE existingCE) { return Overlaps(existingCE, ce); });
}

std::vector<Network> SplitIntoSublayers(const Network& layer)
{
	std::vector<Network> sublayers{ {} };
	for (CE ce : layer)
	{
		bool foundLayer = false;
		for (Network& sublayer : sublayers)
		{
			bool fits = FitsInSublayer(sublayer, ce);
			if (!fits) continue;
			sublayer.push_back(ce);
			foundLayer = true;
			break;
		}

		if (!foundLayer) sublayers.push_back({ ce });
	}
	return sublayers;
}

int ToScreenX(float x, int padding = 40)
{
	return padding + x * (GetScreenWidth() - padding * 2);
}

int ToScreenY(float y, int padding = 40)
{
	return padding + y * (GetScreenHeight() - padding * 2);
}

void DrawNetwork(const Network& network, uint8_t N)
{
	// === Parameters ===
	int layerSeparation = 5;
	// ==================

	std::vector<Network> layers = SplitByLayers(network);

	size_t numSublayers = 0;
	std::vector<std::vector<Network>> allSublayers;
	for (const auto& layer : layers)
	{
		std::vector<Network> sublayers = SplitIntoSublayers(layer);
		allSublayers.push_back(sublayers);
		numSublayers += sublayers.size();
	}

	int maxX = (numSublayers - 1) + (layers.size() - 1) * layerSeparation;

	// Draw channels
	for (uint8_t channel = 0; channel < N; channel++)
	{
		int screenY = ToScreenY((float)channel / (N - 1));
		int startX = ToScreenX(0.0f);
		int endX = ToScreenX(1.0f);
		DrawLine(startX, screenY, endX, screenY, LIGHTGRAY);
	}

	// Draw network
	int xPos = 0;
	for (const auto& layer : allSublayers)
	{
		for (const auto& sublayer : layer)
		{
			for (CE ce : sublayer)
			{
				int screenX = ToScreenX((float)xPos / maxX);
				int screenLoY = ToScreenY((float)ce.lo / (N - 1));
				int screenHiY = ToScreenY((float)ce.hi / (N - 1));
				DrawCircle(screenX, screenLoY, 4.0f, BLACK);
				DrawCircle(screenX, screenHiY, 4.0f, BLACK);
				DrawLine(screenX, screenLoY, screenX, screenHiY, BLACK);
			}
			xPos++;
		}
		xPos += layerSeparation;
	}

	// Metadata text
	std::string networkMeta = std::format("({},{})", network.size(), layers.size());
	DrawText(networkMeta.c_str(), 5, 5, 30, BLACK);
}
#pragma once
#include "Types/Network.h"
#include "SorterHunter.h"

void MainUILoop(std::unique_ptr<SorterHunter>& hunter);
void DrawNetwork(const Network& network, uint8_t N);
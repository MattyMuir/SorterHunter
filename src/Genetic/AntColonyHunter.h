#pragma once
#include "SorterHunter.h"

class AntColonyHunter : public SorterHunter
{
public:
	AntColonyHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_);

protected:

};
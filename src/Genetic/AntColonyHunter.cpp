#include "AntColonyHunter.h"

AntColonyHunter::AntColonyHunter(const PrefixGenerator& prefixGenerator_, const Network& postfix_, const std::vector<CE>& alphabet_)
	: SorterHunter(prefixGenerator_, postfix_, alphabet_)
{}
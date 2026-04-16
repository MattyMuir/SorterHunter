#include "CEGraph.h"
#include "Utility/GlobalRandom.h"

float CEGraph::ReadWeight(size_t ceIdx0, size_t ceIdx1) const
{
	return samplers[ceIdx0].Read(LocalIndex(ceIdx0, ceIdx1));
}

void CEGraph::SetWeight(size_t ceIdx0, size_t ceIdx1, float weight)
{
	samplers[ceIdx0].Update(LocalIndex(ceIdx0, ceIdx1), weight);
}

size_t CEGraph::WeightedRandomNeighbour(size_t ceIdx) const
{
	size_t localIdx = samplers[ceIdx].Sample(GlobalGen());
	return neighbours[ceIdx][localIdx];
}

void CEGraph::ScaleWeights(float scale)
{
    for (auto& sampler : samplers)
        sampler.Scale(scale);
}

size_t CEGraph::StartIndex() const
{
    return samplers.size() - 2;
}

size_t CEGraph::EndIndex() const
{
    return samplers.size() - 1;
}

size_t CEGraph::LocalIndex(size_t from, size_t to) const
{
	return localIndexMap[from].at(to);
}

static inline bool SharesEndpoint(CE a, CE b)
{
	return a.lo == b.lo || a.lo == b.hi || a.hi == b.lo || a.hi == b.hi;
}

static inline CE FlipCE(CE ce, uint8_t N)
{
    return { (uint8_t)(N - 1 - ce.hi), (uint8_t)(N - 1 - ce.lo) };
}

CEGraph BuildCEGraph(uint8_t N, bool symmetric, const std::vector<CE>& alphabet)
{
    size_t numCEs = alphabet.size();
    size_t startVertexIdx = numCEs;
    size_t endVertexIdx = numCEs + 1;
    size_t numVertices = numCEs + 2;

    // Initialize graph storage
    CEGraph graph;
    graph.neighbours.resize(numVertices);
    graph.localIndexMap.resize(numVertices);

    // Loop over all ordered CE pairs
    for (size_t aIdx = 0; aIdx < numCEs - 1; aIdx++)
    {
        for (size_t bIdx = aIdx + 1; bIdx < numCEs; bIdx++)
        {
            CE a = alphabet[aIdx];
            CE b = alphabet[bIdx];

            // Skip edges that share an endpoint
            if (SharesEndpoint(a, b)) continue;
            if (symmetric && SharesEndpoint(a, FlipCE(b, N))) continue;

            size_t localIdx = graph.neighbours[aIdx].size();
            graph.neighbours[aIdx].push_back(bIdx);
            graph.localIndexMap[aIdx].emplace(bIdx, localIdx);
        }
    }

    // Wire up start -> every CE node, and every CE node -> end
    for (size_t ceIdx = 0; ceIdx < numCEs; ceIdx++)
    {
        // start -> i
        graph.localIndexMap[startVertexIdx].emplace(ceIdx, graph.neighbours[startVertexIdx].size());
        graph.neighbours[startVertexIdx].push_back(ceIdx);

        // i -> end
        graph.localIndexMap[ceIdx].emplace(endVertexIdx, graph.neighbours[ceIdx].size());
        graph.neighbours[ceIdx].push_back(endVertexIdx);
    }

    // Build one sampler per node, sized to its out-degree
    graph.samplers.reserve(numVertices);
    for (size_t vertexIdx = 0; vertexIdx < numVertices; vertexIdx++)
        graph.samplers.emplace_back(graph.neighbours[vertexIdx].size());

    return graph;
}
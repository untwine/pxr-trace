// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./aggregateTree.h"

#include "./aggregateTreeBuilder.h"
#include "./collection.h"
#include "./eventTree.h"

#include <stack>

namespace pxr {

TraceAggregateTree::TraceAggregateTree()
{
    Clear();
}

void
TraceAggregateTree::Clear()
{
    _root = TraceAggregateNode::New();
    _eventTimes.clear();
    _counters.clear();
    _counterIndexMap.clear();
    _counterIndex = 0;
}

int
TraceAggregateTree::GetCounterIndex(const TfToken &key) const
{
    _CounterIndexMap::const_iterator it = _counterIndexMap.find(key);
    return it != _counterIndexMap.end() ? it->second : -1;
}

bool
TraceAggregateTree::AddCounter(const TfToken &key, int index, double totalValue)
{
    // Don't add counters with invalid indices
    if (!TF_VERIFY(index >= 0)) {
        return false;
    }

    // We don't expect a counter entry to exist with this key
    if (!TF_VERIFY(_counters.find(key) == _counters.end())) {
        return false;
    }

    // We also don't expect the given index to be used by a different counter
    for (const _CounterIndexMap::value_type& it : _counterIndexMap) {
        if (!TF_VERIFY(it.second != index)) {
            return false;
        }
    }

    // Add the new counter
    _counters[key] = totalValue;
    _counterIndexMap[key] = index;

    return true;
}

void
TraceAggregateTree::Append(
    const TraceEventTreeRefPtr& eventTree, const TraceCollection& collection)
{
    Trace_AggregateTreeBuilder::AddEventTreeToAggregate(
        this, eventTree, collection);
}

}  // namespace pxr
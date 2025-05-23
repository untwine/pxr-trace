// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./collection.h"

namespace pxr {

void TraceCollection::AddToCollection(const TraceThreadId& id, EventListPtr&& events)
{
    EventTable::iterator it = _eventsPerThread.find(id);
    if (it == _eventsPerThread.end()) {
        _eventsPerThread.emplace(id, std::move(events));
    } else {
        it->second->Append(std::move(*events));
    }
}

template <class I>
void TraceCollection::_IterateEvents(Visitor& visitor,
    KeyTokenCache& cache,
    const TraceThreadId& threadIndex, 
    I begin,
    I end) const {

    for (I iter = begin; 
        iter != end; ++iter){
        const TraceEvent& e = *iter;
        if (visitor.AcceptsCategory(e.GetCategory())) {
            // Create the token from the hash using a cache because there 
            // are likely to be many duplicate keys.
            KeyTokenCache::const_iterator it = cache.find(e.GetKey());
            if (it == cache.end()) {
                it = cache.insert(
                    std::make_pair(e.GetKey(),
                        TfToken(e.GetKey()._ptr->GetString()))).first;
            }
            visitor.OnEvent(threadIndex, it->second, e);
        }
    }
}

void 
TraceCollection::_Iterate(Visitor& visitor, bool doReverse) const {
    KeyTokenCache cache;
    visitor.OnBeginCollection();
    for (const EventTable::value_type& i : _eventsPerThread) {
        const TraceThreadId& threadIndex = i.first;
        const EventListPtr &events = i.second;
        visitor.OnBeginThread(threadIndex);
        
        if (doReverse) {
            _IterateEvents(visitor, cache, 
                threadIndex, events->rbegin(), events->rend());
        }
        else {
            _IterateEvents(visitor, cache, 
                threadIndex, events->begin(), events->end());
        }

        visitor.OnEndThread(threadIndex);
    }
    visitor.OnEndCollection();
}

void 
TraceCollection::Iterate(Visitor& visitor) const {
    _Iterate(visitor, false);
}

void 
TraceCollection::ReverseIterate(Visitor& visitor) const {
    _Iterate(visitor, true);
}

TraceCollection::Visitor::~Visitor() {}

}  // namespace pxr

// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./collector.h"

#include "./collection.h"
#include "./collectionNotice.h"
#include "./reporter.h"
#include "./trace.h"

#include <pxr/arch/stackTrace.h>
#include <pxr/arch/timing.h>

#include <pxr/tf/getenv.h>
#include <pxr/tf/instantiateSingleton.h>

#ifdef PXR_PYTHON_SUPPORT_ENABLED
#include <pxr/tf/pyUtils.h>
#endif // PXR_PYTHON_SUPPORT_ENABLED

#include <pxr/tf/staticData.h>
#include <pxr/tf/stringUtils.h>

#include <functional>
#include <iostream>
#include <numeric>
#include <utility>

using std::string;
using std::vector;
using std::make_pair;

namespace pxr {

TF_INSTANTIATE_SINGLETON(TraceCollector);

std::atomic<int> TraceCollector::_isEnabled(0);

TraceCollector::_PerThreadData* TraceCollector::_GetThreadData() noexcept
{
    // Use nullptr initialization as sentinel to prevent guard variable from 
    // being emitted.
    static thread_local TraceCollector::_PerThreadData* threadData = nullptr;
    if (ARCH_UNLIKELY(!threadData)) {
        threadData = &(*_allPerThreadData.Insert());
    }
    return threadData;
}

static
void _OutputGlobalReport()
{
    TraceReporter::GetGlobalReporter()->Report(std::cout);
}

TraceCollector::TraceCollector()
    : _label("TraceRegistry global collector")
    , _measuredScopeOverhead(0)
#ifdef PXR_PYTHON_SUPPORT_ENABLED
    , _isPythonTracingEnabled(false)
#endif // PXR_PYTHON_SUPPORT_ENABLED
{
    TfSingleton<TraceCollector>::SetInstanceConstructed(*this);
    
    // Temporarily enable measurement to find the scope overhead, then disable
    // and clear.
    SetEnabled(true);
    _MeasureScopeOverhead();
    SetEnabled(false);
    Clear();
    
    const bool globalTracing = TfGetenvBool("PXR_ENABLE_GLOBAL_TRACE", false);

#ifdef PXR_PYTHON_SUPPORT_ENABLED
    const bool globalPyTracing =
        TfGetenvBool("PXR_ENABLE_GLOBAL_PY_TRACE", false);
    if (globalPyTracing || globalTracing) {
#else
    if (globalTracing) {
#endif // PXR_PYTHON_SUPPORT_ENABLED
        atexit(_OutputGlobalReport);
        SetEnabled(true);

#ifdef PXR_PYTHON_SUPPORT_ENABLED
        if (globalPyTracing)
            SetPythonTracingEnabled(true);
#endif // PXR_PYTHON_SUPPORT_ENABLED
    }
}


TraceCollector::~TraceCollector()
{
    SetEnabled(false);
}

void
TraceCollector::SetEnabled(bool isEnabled)
{
    _isEnabled.store((int)isEnabled, std::memory_order_release);
}


void
TraceCollector::Scope(
    const TraceKey& key, TimeStamp start, TimeStamp stop) noexcept
{
    _PerThreadData *threadData = GetInstance()._GetThreadData();
    threadData->EmplaceEvent(
        TraceEvent::Timespan, key, start, stop, DefaultCategory::GetId());
}

TraceCollector::TimeStamp
TraceCollector::_BeginEvent(const Key& key, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace", "TraceCollector::BeginEvent");
    if (!IsEnabled()) {
        return 0;
    }

    _PerThreadData *threadData = _GetThreadData();
    return threadData->BeginEvent(key, cat);
}

TraceCollector::TimeStamp
TraceCollector::_EndEvent(const Key& key, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace", "TraceCollector::EndEvent (key)");
    if (!IsEnabled()) {
        return 0;
    }

    _PerThreadData *threadData = _GetThreadData();
    return threadData->EndEvent(key, cat);
}

TraceCollector::TimeStamp
TraceCollector::_MarkerEvent(const Key& key, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace", "TraceCollector::MarkerEvent");
    if (!IsEnabled()) {
        return 0;
    }

    _PerThreadData *threadData = _GetThreadData();
    return threadData->MarkerEvent(key, cat);
}

void
TraceCollector::_EndEventAtTime(const Key& key, double ms, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace",
        "TraceCollector::EndEventAtTime (key, double)");
    if (!IsEnabled()) {
        return;
    }
    
    _PerThreadData *threadData = _GetThreadData();
    threadData->EndEventAtTime(key, ms, cat);
}

void
TraceCollector::_BeginEventAtTime(const Key& key, double ms, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace",
        "TraceCollector::BeginEventAtTime (key, double)");
    if (!IsEnabled()) {
        return;
    }

    _PerThreadData *threadData = _GetThreadData();
    threadData->BeginEventAtTime(key, ms, cat);
}

void
TraceCollector::_MarkerEventAtTime(const Key& key, double ms, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace",
        "TraceCollector::MarkerEventAtTime (key, double)");
    if (!IsEnabled()) {
        return;
    }

    _PerThreadData *threadData = _GetThreadData();
    threadData->MarkerEventAtTime(key, ms, cat);
}

TraceCollector::TimeStamp
TraceCollector::GetScopeOverhead() const
{
    return _measuredScopeOverhead;
}

void
TraceCollector::Clear()
{
    for (_PerThreadData& i : _allPerThreadData) {
        i.Clear();
    }
}

void
TraceCollector::_EndScope(const TraceKey& key, TraceCategoryId cat)
{
    // Note we're not calling _NewEvent, be fast and don't
    // need to cache key
    _PerThreadData *threadData = _GetThreadData();
    threadData->EndScope(key, cat);
}

// An externally visible variable used only to ensure the compiler cannot
// optimize out certain operations for the purposes of overhead measurement.
uint64_t externallyVisibleValue;

void
TraceCollector::_MeasureScopeOverhead()
{
    uint64_t *escape = &externallyVisibleValue;

    _measuredScopeOverhead = ArchMeasureExecutionTime(
        [escape]() {
            TRACE_FUNCTION();
            (*escape)++;
        });
}

void
TraceCollector::CreateCollection() {
    std::unique_ptr<TraceCollection> collection(new TraceCollection());
    for (_PerThreadData& i : _allPerThreadData) {
        TraceCollection::EventListPtr collData = i.GetCollectionData();
        if (!collData->IsEmpty()) {
            collection->AddToCollection(i.GetThreadId(), std::move(collData));
        }
    }

    TraceCollectionAvailable notice(std::move(collection));
    notice.Send();
}

////////////////////////////////////////////////////////////////////////
// Python tracing support.

#ifdef PXR_PYTHON_SUPPORT_ENABLED

static inline TraceCollector::Key
_MakePythonScopeKey(const TfPyTraceInfo& info)
{
    string keyString = TfStringPrintf("%s() (py) in %s:%d (%s)",
                                      info.funcName,
                                      TfGetBaseName(info.fileName).c_str(),
                                      info.funcLine,
                                      info.fileName);
    
    return TraceCollector::Key(keyString);
}

inline void
TraceCollector::_PyTracingCallback(const TfPyTraceInfo& info)
{
    if (info.what == PyTrace_CALL) {
        // If this is a CALL, push a scope for this \a frame in the collector.
        _PerThreadData *threadData = _GetThreadData();
        threadData->PushPyScope(_MakePythonScopeKey(info), IsEnabled());
    } else if (info.what == PyTrace_RETURN) {
        // If instead this is a RETURN, pop the current scope in the collector.
        // We may be called with no active scopes if python tracing is enabled
        // in the middle of some call stack and then the code returns out of
        // whatever frame it was in when tracing got enabled, so just do nothing
        // if there are no active scopes.
        _PerThreadData *threadData = _GetThreadData();
        threadData->PopPyScope(IsEnabled());
    }
}

void
TraceCollector::SetPythonTracingEnabled(bool enabled)
{
    static tbb::spin_mutex enableMutex;
    tbb::spin_mutex::scoped_lock lock(enableMutex);
    
    if (enabled && !IsPythonTracingEnabled()) {
        _isPythonTracingEnabled.store(enabled, std::memory_order_release);
        // Install the python tracing function.
        _pyTraceFnId =
            TfPyRegisterTraceFn([this](const TfPyTraceInfo &info) {
                    _PyTracingCallback(info);
                });
    } else if (!enabled && IsPythonTracingEnabled()) {
        _isPythonTracingEnabled.store(enabled, std::memory_order_release);
        // Remove the python tracing function.
        _pyTraceFnId.reset();
    }
}

#endif // PXR_PYTHON_SUPPORT_ENABLED

////////////////////////////////////////////////////////////////////////
// _PerThreadData methods

TraceCollector::_PerThreadData::_PerThreadData()
    : _writing()
{
    _threadIndex = TraceGetThreadId();
    _events = new EventList();
}

TraceCollector::_PerThreadData::~_PerThreadData()
{
    delete _events.load(std::memory_order_acquire);
}

TraceCollector::TimeStamp
TraceCollector::_PerThreadData::BeginEvent(const Key& key, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace", "TraceCollector::_PerThreadData::BeginEvent");
    AtomicRef lock(_writing);
    EventList* events = _events.load(std::memory_order_acquire);
    const TraceEvent& event = 
        events->EmplaceBack(TraceEvent::Begin, events->CacheKey(key), cat);
    return event.GetTimeStamp();
}

TraceCollector::TimeStamp
TraceCollector::_PerThreadData::EndEvent(const Key& key, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace", "TraceCollector::_PerThreadData::EndEvent");
    AtomicRef lock(_writing);
    EventList* events = _events.load(std::memory_order_acquire);
    const TraceEvent& event =
        events->EmplaceBack(TraceEvent::End, events->CacheKey(key), cat);
    return event.GetTimeStamp();
}

TraceCollector::TimeStamp
TraceCollector::_PerThreadData::MarkerEvent(const Key& key, TraceCategoryId cat)
{
    TfAutoMallocTag2 tag("Trace", "TraceCollector::_PerThreadData::MarkerEvent");
    AtomicRef lock(_writing);
    EventList* events = _events.load(std::memory_order_acquire);
    const TraceEvent& event =
        events->EmplaceBack(TraceEvent::Marker, events->CacheKey(key), cat);
    return event.GetTimeStamp();
}

void
TraceCollector::_PerThreadData::BeginEventAtTime(
    const Key& key, double ms, TraceCategoryId cat)
{
    AtomicRef lock(_writing);
    TfAutoMallocTag2 tag("Trace", 
        "TraceCollector::_PerThreadData::BeginEventAtTime");
    const TimeStamp ts = 
        uint64_t(ms * 1000 / double(ArchTicksToSeconds(1000000)));
    EventList* events = _events.load(std::memory_order_acquire);
    events->EmplaceBack(TraceEvent::Begin, events->CacheKey(key), ts, cat);
}

void
TraceCollector::_PerThreadData::EndEventAtTime(
    const Key& key, double ms, TraceCategoryId cat)
{
    AtomicRef lock(_writing);
    TfAutoMallocTag2 tag("Trace", 
        "TraceCollector::_PerThreadData::EndEventAtTime");
    const TimeStamp ts = 
        uint64_t(ms * 1000 / double(ArchTicksToSeconds(1000000)));
    EventList* events = _events.load(std::memory_order_acquire);
    events->EmplaceBack(TraceEvent::End, events->CacheKey(key), ts, cat);
}

void
TraceCollector::_PerThreadData::MarkerEventAtTime(
    const Key& key, double ms, TraceCategoryId cat)
{
    AtomicRef lock(_writing);
    TfAutoMallocTag2 tag("Trace", 
        "TraceCollector::_PerThreadData::MarkerEventAtTime");
    const TimeStamp ts = 
        uint64_t(ms * 1000 / double(ArchTicksToSeconds(1000000)));
    EventList* events = _events.load(std::memory_order_acquire);
    events->EmplaceBack(TraceEvent::Marker, events->CacheKey(key), ts, cat);
}

void
TraceCollector::_PerThreadData::_EndScope(
    const TraceKey& key, TraceCategoryId cat)
{
    _events.load(std::memory_order_acquire)->EmplaceBack(TraceEvent::End, key, cat);
}

void
TraceCollector::_PerThreadData::CounterDelta(
    const Key& key, double value, TraceCategoryId cat)
{
    AtomicRef lock(_writing);
    EventList* events = _events.load(std::memory_order_acquire);
    events->EmplaceBack(
        TraceEvent::CounterDelta, events->CacheKey(key), value, cat);
}

void
TraceCollector::_PerThreadData::CounterValue(
    const Key& key, double value, TraceCategoryId cat)
{
    AtomicRef lock(_writing);
    EventList* events = _events.load(std::memory_order_acquire);
    events->EmplaceBack(
        TraceEvent::CounterValue, events->CacheKey(key), value, cat);
}

#ifdef PXR_PYTHON_SUPPORT_ENABLED

void 
TraceCollector::_PerThreadData::PushPyScope(
    const Key& key, bool enabled)
{
    AtomicRef lock(_writing);
    if (enabled) {
        EventList* events = _events.load(std::memory_order_acquire);
        TraceKey stableKey = events->CacheKey(key);
        _BeginScope(stableKey, TraceCategory::Default);
    }
    _pyScopes.push_back(PyScope{key});
}

void
TraceCollector::_PerThreadData::PopPyScope(bool enabled)
{
    AtomicRef lock(_writing);
    if (!_pyScopes.empty()) {
        if (enabled) {
            const PyScope& scope = _pyScopes.back();
            EventList* events = _events.load(std::memory_order_acquire);
            TraceKey stableKey = events->CacheKey(scope.key);
            _EndScope(stableKey, TraceCategory::Default);
        }
        _pyScopes.pop_back();
    }
}

#endif // PXR_PYTHON_SUPPORT_ENABLED

std::unique_ptr<TraceCollection::EventList>
TraceCollector::_PerThreadData::GetCollectionData() {

    // Create a new event list and atomically swap it with the current list.
    EventList* newEventList = new EventList();

    std::unique_ptr<EventList> prevList(_events.exchange(newEventList));

    // The previous list may have a writer so we have to wait until noone is 
    // potentially writing to prevList.
    while (_writing.load(std::memory_order_acquire)) {}

    // Now it should be ok to release the list to the outside.
    return prevList;
}

void
TraceCollector::_PerThreadData::Clear() {
    // Swap out the data and let the event list be cleaned up.
    GetCollectionData();
}

}  // namespace pxr

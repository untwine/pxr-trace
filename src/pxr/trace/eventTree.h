// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_TRACE_EVENT_TREE_H
#define PXR_TRACE_EVENT_TREE_H

#include "./api.h"
#include "./event.h"
#include "./eventNode.h"
#include "./threads.h"
#include <pxr/tf/refBase.h>
#include <pxr/tf/refPtr.h>
#include <pxr/tf/token.h>
#include <pxr/tf/weakBase.h>
#include <pxr/tf/weakPtr.h>
#include <pxr/tf/declarePtrs.h>

#include <functional>
#include <vector>
#include <unordered_map>

namespace pxr {

class TraceCollection;
class JsWriter;
TF_DECLARE_WEAK_AND_REF_PTRS(TraceEventTree);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceEventTree
///
/// This class contains a timeline call tree and a map of counters to their 
/// values over time.
///
///
class TraceEventTree : public TfRefBase, public TfWeakBase {
public:
    using CounterValues = std::vector<std::pair<TraceEvent::TimeStamp, double>>;
    using CounterValuesMap =
        std::unordered_map<TfToken, CounterValues, TfToken::HashFunctor>;
    using CounterMap =
        std::unordered_map<TfToken, double, TfToken::HashFunctor>;

    using MarkerValues = std::vector<std::pair<TraceEvent::TimeStamp, TraceThreadId>>;
    using MarkerValuesMap =
        std::unordered_map<TfToken, MarkerValues, TfToken::HashFunctor>;

    /// Creates a new TraceEventTree instance from the data in \p collection 
    /// and \p initialCounterValues.
    TRACE_API static TraceEventTreeRefPtr New(
        const TraceCollection& collection,
        const CounterMap* initialCounterValues = nullptr);

    static TraceEventTreeRefPtr New() {
        return TfCreateRefPtr( 
            new TraceEventTree(TraceEventNode::New()));
    }

    static TraceEventTreeRefPtr New(
            TraceEventNodeRefPtr root, 
            CounterValuesMap counters,
            MarkerValuesMap markers) {
        return TfCreateRefPtr( 
            new TraceEventTree(root, std::move(counters), std::move(markers)));
    }

    /// Returns the root node of the tree.
    const TraceEventNodeRefPtr& GetRoot() const { return _root; }

    /// Returns the map of counter values.
    const CounterValuesMap& GetCounters() const { return _counters; }

    /// Returns the map of markers values.
    const MarkerValuesMap& GetMarkers() const { return _markers; }

    /// Return the final value of the counters in the report.
    CounterMap GetFinalCounterValues() const;

    /// Writes a JSON object representing the data in the call tree that 
    /// conforms to the Chrome Trace format.
    using ExtraFieldFn = std::function<void(JsWriter&)>;
    TRACE_API void WriteChromeTraceObject(
        JsWriter& writer, ExtraFieldFn extraFields = ExtraFieldFn()) const;

    /// Adds the contexts of \p tree to this tree.
    TRACE_API void Merge(const TraceEventTreeRefPtr& tree);

    /// Adds the data from \p collection to this tree.
    TRACE_API TraceEventTreeRefPtr Add(const TraceCollection& collection);

private:
    TraceEventTree(TraceEventNodeRefPtr root)
        : _root(root) {}

    TraceEventTree(  TraceEventNodeRefPtr root, 
                            CounterValuesMap counters,
                            MarkerValuesMap markers)
        : _root(root)
        , _counters(std::move(counters))
        , _markers(std::move(markers)) {}

    // Root of the call tree.
    TraceEventNodeRefPtr _root;
    // Counter data of the trace.
    CounterValuesMap _counters;
    // Marker data of the trace.
    MarkerValuesMap _markers;
};

}  // namespace pxr

#endif // PXR_TRACE_EVENT_TREE_H

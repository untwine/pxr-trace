// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_TRACE_REPORTER_DATA_SOURCE_COLLECTOR_H
#define PXR_TRACE_REPORTER_DATA_SOURCE_COLLECTOR_H

#include "./api.h"
#include "./collection.h"
#include "./collectionNotice.h"
#include "./reporterDataSourceBase.h"

#include <pxr/tf/declarePtrs.h>

#include <tbb/concurrent_queue.h>

#include <vector>

namespace pxr {

TF_DECLARE_WEAK_PTRS(TraceReporterDataSourceCollector);

////////////////////////////////////////////////////////////////////////////////
/// \class TraceReporterDataSourceCollector
///
/// This class is an implementation of TraceReporterDataSourceBase which 
/// retrieves TraceCollections from the TraceCollector singleton.
///
class TraceReporterDataSourceCollector :
    public TraceReporterDataSourceBase, public TfWeakBase {
public:
    using This = TraceReporterDataSourceCollector;
    using ThisPtr = TraceReporterDataSourceCollectorPtr;
    using ThisRefPtr = std::unique_ptr<This>;

    /// Creates a new TraceReporterDataSourceCollector.
    static ThisRefPtr New() {
        return ThisRefPtr(new This());
    }

    /// Creates a new TraceReporterDataSourceCollector which will only listen to
    /// the TraceCollectionAvailable notice when \p accept returns true. 
    /// \p accept must be thread-safe.
    static ThisRefPtr New(std::function<bool()> accept) {
        return ThisRefPtr(new This(std::move(accept)));
    }

    /// Removes all references to TraceCollections.
    TRACE_API void Clear() override;

    /// Returns the next TraceCollections which need to be processed.
    TRACE_API std::vector<CollectionPtr> ConsumeData() override;

private:
    TRACE_API TraceReporterDataSourceCollector();
    TRACE_API TraceReporterDataSourceCollector(std::function<bool()> accept);

    // Add the new collection to the pending queue.
    void _OnTraceCollection(const TraceCollectionAvailable&);

    std::function<bool()> _accept;
    tbb::concurrent_queue<CollectionPtr> _pendingCollections;
};

}  // namespace pxr

#endif // PXR_TRACE_REPORTER_DATA_SOURCE_COLLECTOR_H
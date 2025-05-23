// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_TRACE_SERIALIZATION_H
#define PXR_TRACE_SERIALIZATION_H

#include "./api.h"
#include "./collection.h"

#include <istream>
#include <ostream>
#include <memory>
#include <vector>

namespace pxr {

////////////////////////////////////////////////////////////////////////////////
/// \class TraceSerialization
///
/// This class contains methods to read and write TraceCollection.
///
class TraceSerialization {
public:
    /// Writes \p col to \p ostr.
    /// Returns true if the write was successful, false otherwise.
    TRACE_API static bool Write(std::ostream& ostr,
        const std::shared_ptr<TraceCollection>& col);

    /// Writes \p collections to \p ostr.
    /// Returns true if the write was successful, false otherwise.
    TRACE_API static bool Write(
        std::ostream& ostr,
        const std::vector<std::shared_ptr<TraceCollection>>& collections);

    /// Tries to create a TraceCollection from the contexts of \p istr.
    /// Returns a pointer to the created collection if it was successful.
    /// If there is an error reading \p istr, \p error will be populated with a
    /// description.
    TRACE_API static std::unique_ptr<TraceCollection> Read(std::istream& istr,
        std::string* error = nullptr);
};

}  // namespace pxr

#endif // PXR_TRACE_SERIALIZATION_H
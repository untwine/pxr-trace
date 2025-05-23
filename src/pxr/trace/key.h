// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_TRACE_KEY_H
#define PXR_TRACE_KEY_H

#include "./staticKeyData.h"


namespace pxr {

////////////////////////////////////////////////////////////////////////////////
/// \class TraceKey
///
/// A wrapper around a TraceStaticKeyData pointer that is stored in TraceEvent 
/// instances.
///
class TraceKey {
public:
    /// Constructor.
    constexpr TraceKey(const TraceStaticKeyData& data) : _ptr(&data) {}

    /// Equality comparison.
    bool operator == (const TraceKey& other) const {
        if (_ptr == other._ptr) {
            return true;
        } else {
            return *_ptr == *other._ptr;
        }
    }

    /// Hash function.
    size_t Hash() const {
        return reinterpret_cast<size_t>(_ptr)/sizeof(TraceStaticKeyData);
    }

    /// A Hash functor which may be used to store keys in a TfHashMap.
    struct HashFunctor {
        size_t operator()(const TraceKey& key) const {
            return key.Hash();
        }
    };

private:
    const TraceStaticKeyData* _ptr;

    // TraceCollection converts TraceKeys to TfTokens for visitors.
    friend class TraceCollection;
};

}  // namespace pxr

#endif // PXR_TRACE_KEY_H

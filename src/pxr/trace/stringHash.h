// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_TRACE_STRING_HASH_H
#define PXR_TRACE_STRING_HASH_H

#include <cstdint>

namespace pxr {

///////////////////////////////////////////////////////////////////////////////
///
/// \class TraceStringHash
///
/// This class provides a function to compute compile time hashes for string 
/// literals.
///
class TraceStringHash {
  public:

    /// Computes a compile time hash of \p str.
    template <int N>
    static constexpr std::uint32_t Hash(const char (&str)[N]) {
        return djb2HashStr<N-1>(str);
    }

  private:
    // Recursive function computing the xor variant of the djb2 hash
    // function.
    template <int N>
    static constexpr std::uint32_t djb2HashStr(const char* str) {
        return (djb2HashStr<N-1>(str) * 33) ^ str[N-1];
    }
};

// Template recursion base case.
template <>
constexpr std::uint32_t TraceStringHash::djb2HashStr<0>(const char* str) {
    return 5381;
}

}  // namespace pxr

#endif //PXR_TRACE_STRING_HASH_H
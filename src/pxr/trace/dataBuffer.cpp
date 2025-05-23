// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./dataBuffer.h"

#include <algorithm>

namespace pxr {

constexpr size_t _GetMaxAlign() {
    // Our version of GCC does not have std::max_align_t
    using namespace std;
    return alignof(max_align_t);
}

void
TraceDataBuffer::Allocator::AllocateBlock(const size_t align, 
                                         const size_t desiredSize)
{
    const size_t maxAlign = _GetMaxAlign();
    const size_t blockSize = 
        std::max(align > maxAlign ? (align + desiredSize) : desiredSize,
            _desiredBlockSize);
    BlockPtr block(new Byte[ blockSize ]);
    _next = block.get();
    _blockEnd = _next + blockSize;
    _blocks.push_back(std::move(block));
}

}  // namespace pxr

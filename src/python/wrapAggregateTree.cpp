// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/trace/aggregateTree.h>

#include <pxr/tf/makePyConstructor.h>
#include <pxr/tf/pyPtrHelpers.h>

#include <pxr/boost/python/class.hpp>

using namespace pxr;

using namespace pxr::boost::python;

void wrapAggregateTree()
{
    using This = TraceAggregateTree;
    using ThisPtr = TraceAggregateTreePtr;

    class_<This, ThisPtr>("AggregateTree", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(&This::New))
        .add_property("root", &This::GetRoot)
    ;
}

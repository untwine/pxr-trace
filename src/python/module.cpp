// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/tf/pyModule.h>

using namespace pxr;

TF_WRAP_MODULE
{
    TF_WRAP( Collector );
    TF_WRAP( AggregateNode );
    TF_WRAP( AggregateTree );
    TF_WRAP( Reporter );

    TF_WRAP( TestTrace );
}

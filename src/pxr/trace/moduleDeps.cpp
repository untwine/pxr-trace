// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.
////////////////////////////////////////////////////////////////////////

#include "pxr/trace/pxr.h"
#include "pxr/tf/registryManager.h"
#include "pxr/tf/scriptModuleLoader.h"
#include "pxr/tf/token.h"

#include <vector>

TRACE_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfScriptModuleLoader) {
    // List of direct dependencies for this library.
    const std::vector<TfToken> reqs = {
        TfToken("arch"),
        TfToken("js"),
        TfToken("tf"),
    };
    TfScriptModuleLoader::GetInstance().
    RegisterLibrary(TfToken("trace"), TfToken("pxr.Trace"), reqs);
}

TRACE_NAMESPACE_CLOSE_SCOPE

// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./serialization.h"

#include "./jsonSerialization.h"

#include <pxr/tf/scopeDescription.h>
#include <pxr/js/json.h>

namespace pxr {

bool 
TraceSerialization::Write(
    std::ostream& ostr, const std::shared_ptr<TraceCollection>& collection) 
{
    if (!collection) {
        return false;
    }
    return Write(
        ostr, std::vector<std::shared_ptr<TraceCollection>>{collection});
}

bool
TraceSerialization::Write(
    std::ostream& ostr,
    const std::vector<std::shared_ptr<TraceCollection>>& collections)
{
    JsValue colVal;
    if (collections.empty()) {
        return false;
    }
    {
        TF_DESCRIBE_SCOPE("Writing JSON");
        JsWriter js(ostr);
        Trace_JSONSerialization::WriteCollectionsToJSON(js, collections);
        return true;
    }
    return false;
}

std::unique_ptr<TraceCollection>
TraceSerialization::Read(std::istream& istr, std::string* errorStr)
{
    JsParseError error;
    JsValue value = JsParseStream(istr, &error);
    if (value.IsNull()) {
        if (errorStr) {
            *errorStr = TfStringPrintf("Error parsing JSON\n"
                "line: %d, col: %d ->\n\t%s.\n",
                error.line, error.column,
                error.reason.c_str());
        }
        return nullptr;
    }
    return Trace_JSONSerialization::CollectionFromJSON(value);
}

}  // namespace pxr

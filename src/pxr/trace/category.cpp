// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./category.h"

#include <pxr/tf/instantiateSingleton.h>

namespace pxr {

TF_INSTANTIATE_SINGLETON(TraceCategory);

TraceCategory::TraceCategory()
{
    RegisterCategory(TraceCategory::Default, "Default");
}

TraceCategory&
TraceCategory::GetInstance() {
    return TfSingleton<TraceCategory>::GetInstance();
}

void 
TraceCategory::RegisterCategory(TraceCategoryId id, const std::string& name)
{
    _idToNames.insert(std::make_pair(id, name));
}

std::vector<std::string>
TraceCategory::GetCategories(TraceCategoryId id) const
{
    std::vector<std::string> result;
    using const_iter = 
        std::multimap<TraceCategoryId, std::string>::const_iterator;
    std::pair<const_iter, const_iter> range = _idToNames.equal_range(id);
    for (const_iter i = range.first; i != range.second; ++i) {
        result.push_back(i->second);
    }
    return result;
}

}  // namespace pxr

// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/trace/trace.h>
#include <pxr/trace/reporter.h>
#include <pxr/trace/collector.h>

#include <pxr/boost/python/def.hpp>
#include <pxr/boost/python/enum.hpp>
#include <pxr/boost/python/list.hpp>

#include <chrono>
#include <string>
#include <thread>

using namespace std::chrono_literals;
using namespace pxr;

using namespace pxr::boost::python;

// ----------------------------------------
// A set of functions using TRACE_FUNCTION, TRACE_SCOPE

static void
TestNestingFunc2()
{
    TRACE_FUNCTION();
    std::this_thread::sleep_for(1us);
}

static void
TestNestingFunc3()
{
    TRACE_FUNCTION();
    std::this_thread::sleep_for(1us);
    TRACE_SCOPE("Foo");
    std::this_thread::sleep_for(1us);

    TraceCollector& gc = TraceCollector::GetInstance();
    TF_UNUSED(gc);
}

static void
TestNestingFunc1()
{
    TRACE_FUNCTION();
    std::this_thread::sleep_for(1us);

    TestNestingFunc2();
    TestNestingFunc3();
}

static void
TestNesting()
{
    TRACE_FUNCTION();
    std::this_thread::sleep_for(1us);

    TestNestingFunc1();
}

// ----------------------------------------
// A set of functions using TraceAuto

static void
TestAutoFunc2()
{
    TraceAuto t(TF_FUNC_NAME());
    std::this_thread::sleep_for(1us);
}

static void
TestAutoFunc3()
{
    TraceAuto t(TF_FUNC_NAME());
    std::this_thread::sleep_for(1us);
}

static void
TestAutoFunc1()
{
    TraceAuto t(TF_FUNC_NAME());
    std::this_thread::sleep_for(1us);

    TestAutoFunc2();
    TestAutoFunc3();
}

static void
TestAuto()
{
    TraceAuto t(TF_FUNC_NAME());
    std::this_thread::sleep_for(1us);

    TestAutoFunc1();
}

static std::string
TestEventName()
{
    return "C_PLUS_PLUS_EVENT";
}


static void
TestCreateEvents()
{
    TraceCollector* gc = &TraceCollector::GetInstance();
    gc->BeginEvent(TestEventName());
    gc->EndEvent(TestEventName());
    
}

void wrapTestTrace()
{    
    def("TestNesting", &::TestNesting);
    def("TestAuto", &::TestAuto);
    def("TestCreateEvents", &::TestCreateEvents);
    def("GetTestEventName", &::TestEventName);
}


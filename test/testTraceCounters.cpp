// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/trace/trace.h>
#include <pxr/trace/reporter.h>
#include <pxr/trace/eventTree.h>

#include <iostream>

using namespace pxr;

static void
TestCounters() {
    // All deltas
    TRACE_COUNTER_DELTA("Counter A", 1);
    TRACE_COUNTER_DELTA("Counter A", 2);
    TRACE_COUNTER_DELTA("Counter A", 3);

    // All values
    TRACE_COUNTER_VALUE("Counter B", 1);
    TRACE_COUNTER_VALUE("Counter B", 2);
    TRACE_COUNTER_VALUE("Counter B", 3);

    // value then delta
    TRACE_COUNTER_VALUE("Counter C", 5);
    TRACE_COUNTER_DELTA("Counter C", -1);
    TRACE_COUNTER_DELTA("Counter C", -2);

    // deltas then value
    TRACE_COUNTER_DELTA("Counter D", 1);
    TRACE_COUNTER_DELTA("Counter D", 2);
    TRACE_COUNTER_VALUE("Counter D", -5);
}

static void TestTimelineCounterValues(
    const TfToken& counterName, const std::vector<double> desiredValues)
{
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    TraceEventTreeRefPtr timeline = reporter->GetEventTree();
    TF_AXIOM(timeline);

    const TraceEventTree::CounterValuesMap& counters =
        timeline->GetCounters();
    TraceEventTree::CounterValuesMap::const_iterator it =
        counters.find(counterName);
    TF_AXIOM(it != counters.end());

    // The number and order of values should match.
    const TraceEventTree::CounterValues& values = it->second;
    TF_AXIOM(desiredValues.size() == values.size());

    std::vector<double>::const_iterator desiredIt = desiredValues.begin();
    for (auto& v : values) {
        TF_AXIOM(v.second == *desiredIt);
        ++desiredIt;
    }
}

static void TestAggregateCounterValue(
    const TfToken& counterName, const double desiredValue)
{
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    const TraceReporter::CounterMap& counters = reporter->GetCounters();
    TraceReporter::CounterMap::const_iterator it =
        counters.find(counterName);
    TF_AXIOM(it != counters.end());
    TF_AXIOM(it->second == desiredValue);
    
}

static void TestAggregateCounterDelta(
    const TfToken& counterName, const double desiredValue)
{
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    double value = reporter->GetAggregateTreeRoot()->GetInclusiveCounterValue(
        reporter->GetCounterIndex(counterName));
    printf("Node:%s Counter: %s Expected value: %f actual: %f\n",
        reporter->GetAggregateTreeRoot()->GetKey().GetText(),
        counterName.GetText(), desiredValue, value);
    TF_AXIOM(value == desiredValue);
}

int main(int argc, char* argv[]) {
    TraceCollector* collector = &TraceCollector::GetInstance();
    TraceReporterPtr reporter = TraceReporter::GetGlobalReporter();
    collector->SetEnabled(true);
    TestCounters();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Test that the aggregate reporter works correctly.
    TestAggregateCounterValue(TfToken("Counter A"), 6.0);
    TestAggregateCounterDelta(TfToken("Counter A"), 6.0);
    TestAggregateCounterValue(TfToken("Counter B"), 3.0);
    TestAggregateCounterDelta(TfToken("Counter B"), 0.0);
    TestAggregateCounterValue(TfToken("Counter C"), 2.0);
    TestAggregateCounterDelta(TfToken("Counter C"), -3.0);
    TestAggregateCounterValue(TfToken("Counter D"), -5.0);
    TestAggregateCounterDelta(TfToken("Counter D"), 3.0);

    // Test that the timeline reporter works correctly
    TestTimelineCounterValues(TfToken("Counter A"), {1.0,3.0,6.0});
    TestTimelineCounterValues(TfToken("Counter B"), {1.0,2.0,3.0});
    TestTimelineCounterValues(TfToken("Counter C"), {5.0,4.0,2.0});
    TestTimelineCounterValues(TfToken("Counter D"), {1.0,3.0,-5.0});

    collector->SetEnabled(true);
    // A new counter should not affect the reporting oc counters in TestCounters
    TRACE_COUNTER_DELTA("Counter E", 1);
    TestCounters();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Test that the aggregate reporter works correctly.
    TestAggregateCounterValue(TfToken("Counter A"), 12.0);
    TestAggregateCounterDelta(TfToken("Counter A"), 12.0);
    TestAggregateCounterValue(TfToken("Counter B"), 3.0);
    TestAggregateCounterDelta(TfToken("Counter B"), 0.0);
    TestAggregateCounterValue(TfToken("Counter C"), 2.0);
    TestAggregateCounterDelta(TfToken("Counter C"), -6.0);
    TestAggregateCounterValue(TfToken("Counter D"), -5.0);
    TestAggregateCounterDelta(TfToken("Counter D"), 6.0);
    TestAggregateCounterValue(TfToken("Counter E"), 1.0);
    TestAggregateCounterDelta(TfToken("Counter E"), 1.0);

    // Test that the timeline reporter works correctly
    TestTimelineCounterValues(TfToken("Counter A"), {1.0,3.0,6.0,7.0,9.0,12.0});
    TestTimelineCounterValues(TfToken("Counter B"), {1.0,2.0,3.0,1.0,2.0,3.0});
    TestTimelineCounterValues(TfToken("Counter C"), {5.0,4.0,2.0,5.0,4.0,2.0});
    TestTimelineCounterValues(TfToken("Counter D"), {1.0,3.0,-5.0,-4.0,-2.0,-5.0});
    TestTimelineCounterValues(TfToken("Counter E"), {1.0});

    // Make sure clearing works.
    reporter->ClearTree();

    collector->SetEnabled(true);
    TestCounters();
    collector->SetEnabled(false);
    reporter->ReportChromeTracing(std::cout);

    // Test that the aggregate reporter works correctly.
    TestAggregateCounterValue(TfToken("Counter A"), 6.0);
    TestAggregateCounterDelta(TfToken("Counter A"), 6.0);
    TestAggregateCounterValue(TfToken("Counter B"), 3.0);
    TestAggregateCounterDelta(TfToken("Counter B"), 0.0);
    TestAggregateCounterValue(TfToken("Counter C"), 2.0);
    TestAggregateCounterDelta(TfToken("Counter C"), -3.0);
    TestAggregateCounterValue(TfToken("Counter D"), -5.0);
    TestAggregateCounterDelta(TfToken("Counter D"), 3.0);

    // Test that the timeline reporter works correctly
    TestTimelineCounterValues(TfToken("Counter A"), {1.0,3.0,6.0});
    TestTimelineCounterValues(TfToken("Counter B"), {1.0,2.0,3.0});
    TestTimelineCounterValues(TfToken("Counter C"), {5.0,4.0,2.0});
    TestTimelineCounterValues(TfToken("Counter D"), {1.0,3.0,-5.0});


    return 0;
}
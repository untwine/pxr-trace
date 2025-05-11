// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/trace/trace.h>
#include <pxr/trace/eventTree.h>
#include <pxr/trace/reporter.h>
#include <pxr/trace/reporterDataSourceCollector.h>
#include <pxr/trace/reporterDataSourceCollection.h>
#include <pxr/tf/stopwatch.h>
#include <pxr/tf/stringUtils.h>

#include <iostream>

TRACE_NAMESPACE_USING_DIRECTIVE

void
WriteStats(FILE *file, const std::string& name, const TfStopwatch &timer)
{
    fprintf(
        file,
        "{'profile':'%s','metric':'time','value':%f,'samples':%zu}\n",
        name.c_str(),
        timer.GetSeconds(),
        timer.GetSampleCount());
}

void Recursion(int N)
{
    TRACE_FUNCTION();
    if (N <= 1) {
        return;
    }
    Recursion(N-1);
}

std::shared_ptr<TraceCollection>
CreateTrace(int N, int R)
{
    std::unique_ptr<TraceReporterDataSourceCollector> dataSrc =
        TraceReporterDataSourceCollector::New();
    TraceCollector::GetInstance().SetEnabled(true);
    TRACE_SCOPE("Test Outer");
    for (int i = 0; i < N/R; i++) {
        Recursion(R);
    }
    TraceCollector::GetInstance().SetEnabled(false);
    std::shared_ptr<TraceCollection> collection = 
        dataSrc->ConsumeData()[0];
    TraceReporter::GetGlobalReporter()->ClearTree();
    return collection;
}

int main(int argc, char* argv[])
{
    FILE *statsFile = fopen("perfstats.raw", "w");
    TfStopwatch watch;

    std::vector<int> recrusionSizes = {1, 2, 10};
    std::vector<int> testSizes = {1000000, 10000000, 100000000};

    // Take any command line arguments and parse to see if a valid test size 
    // index was passed. If there are too many arguments or the arguments are 
    // invalid, they are ignored completely. By default only the first test
    // size is run. Larger sizes better stress the system but heavily increase
    // runtime and memory consumption.
    size_t maxTestSize = 1;
    if (argc == 2) {
        std::stringstream convert(argv[1]);
        size_t tmp;
        if (convert >> tmp) {
            if (tmp <= testSizes.size() && tmp > 0) {
                maxTestSize = tmp;
            }
        }
    }

    for (int R : recrusionSizes) {
        std::cout << "Recursion depth: " << R << std::endl;  
        for (size_t i = 0; i < maxTestSize; ++i) {
            int size = testSizes[i];

            watch.Reset();
            watch.Start();
            auto collection = CreateTrace(size, R);
            watch.Stop();
            std::cout << "Create Trace    N: " << size << " time: " 
                << watch.GetSeconds() << " scopes/msec: " 
                << float(size)/watch.GetMilliseconds()
                << std::endl;
            auto reporter = TraceReporter::New(
                "Test", TraceReporterDataSourceCollection::New(collection));

            watch.Reset();
            watch.Start();

            reporter->UpdateTraceTrees();
            watch.Stop();
            WriteStats( statsFile,
                        TfStringPrintf("trace trees R %d N %d", R, size),
                        watch);
            std::cout << "Trace Trees N: " << size << " time: " 
                << watch.GetSeconds()
                << " scopes/msec: " << float(size)/watch.GetMilliseconds()
                << std::endl;
        }
    }
    fclose(statsFile);
    return 0;
}

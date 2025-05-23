// Copyright 2018 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/trace/trace.h>
#include <pxr/trace/collectionNotice.h>
#include <pxr/trace/serialization.h>

#include <fstream>
#include <iostream>
#include <sstream>

using namespace pxr;

// Declare a custom Trace category
constexpr TraceCategoryId TestCategory 
    = TraceCategory::CreateTraceCategoryId("TestCategory");

static std::unique_ptr<TraceEventList>
CreateTestEvents(double timeOffset)
{
    double ms = .001;
    timeOffset += ms;
    std::unique_ptr<TraceEventList> events(new TraceEventList);

    static constexpr TraceStaticKeyData counterKey("Test Counter");
    {
        TraceEvent counterEvent(
            TraceEvent::CounterDelta,
            counterKey,
            1,
            TraceCategory::Default
        );
        counterEvent.SetTimeStamp(ArchSecondsToTicks(2.0*ms + timeOffset));
        events->EmplaceBack(std::move(counterEvent));
    }

    events->EmplaceBack(
        TraceEvent::Begin,
        events->CacheKey("Inner Scope 2"),
        ArchSecondsToTicks(3.0*ms + timeOffset),
        TestCategory
    );
    events->EmplaceBack(
        TraceEvent::End,
        events->CacheKey("Inner Scope 2"),
        ArchSecondsToTicks(4.0*ms + timeOffset),
        TestCategory
    );

    {
        bool data = true;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 0"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(ArchSecondsToTicks(5.0*ms + timeOffset));
        events->EmplaceBack(std::move(dataEvent));
    }
    {
        int data = -2;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 1"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(ArchSecondsToTicks(6.0*ms + timeOffset));
        events->EmplaceBack(std::move(dataEvent));
    }
    {
        uint64_t data = ~0;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 2"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(ArchSecondsToTicks(7.0*ms + timeOffset));
        events->EmplaceBack(std::move(dataEvent));
    }
    {
        double data = 1.5;
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 3"),
            data,
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(ArchSecondsToTicks(8.0*ms + timeOffset));
        events->EmplaceBack(std::move(dataEvent));
    }
    {
        std::string data = "String Data";
        TraceEvent dataEvent(
            TraceEvent::Data,
            events->CacheKey("Test Data 4"),
            events->StoreData(data.c_str()),
            TraceCategory::Default
        );
        dataEvent.SetTimeStamp(ArchSecondsToTicks(9.0*ms + timeOffset));
        events->EmplaceBack(std::move(dataEvent));
    }

    static constexpr TraceStaticKeyData keyInner("InnerScope");
    events->EmplaceBack(TraceEvent::Timespan, keyInner,
        ArchSecondsToTicks(ms + timeOffset),
        ArchSecondsToTicks(10.0*ms + timeOffset),
        TraceCategory::Default);


    {
        TraceEvent counterEvent(
            TraceEvent::CounterDelta,
            counterKey,
            1,
            TraceCategory::Default
        );
        counterEvent.SetTimeStamp(ArchSecondsToTicks(11.0*ms + timeOffset));
        events->EmplaceBack(std::move(counterEvent));
    }
    {
        TraceEvent counterEvent(
            TraceEvent::CounterValue,
            counterKey,
            -1,
            TraceCategory::Default
        );
        counterEvent.SetTimeStamp(ArchSecondsToTicks(12.0*ms + timeOffset));
        events->EmplaceBack(std::move(counterEvent));
    }
    static constexpr TraceStaticKeyData keyOuter("OuterScope");
    events->EmplaceBack(TraceEvent::Timespan, keyOuter,
        ArchSecondsToTicks(0 + timeOffset),
        ArchSecondsToTicks(13.0*ms + timeOffset),
        TraceCategory::Default);

    events->EmplaceBack(
        TraceEvent::Marker,
        events->CacheKey("Test Marker 1"),
        ArchSecondsToTicks(4.0*ms  + timeOffset),
        TraceCategory::Default
    );

    events->EmplaceBack(
        TraceEvent::Marker,
        events->CacheKey("Test Marker 2"),
        ArchSecondsToTicks(5.0*ms + timeOffset),
        TraceCategory::Default
    );
    return events;
}

static std::unique_ptr<TraceCollection>
CreateTestCollection(float startTimeSec=0.0)
{
    std::unique_ptr<TraceCollection> collection(new TraceCollection);
    collection->AddToCollection(
        TraceThreadId("Thread 1"),
        CreateTestEvents(startTimeSec));
    collection->AddToCollection(TraceThreadId("Thread 2"), 
        CreateTestEvents(startTimeSec + 0.001));
    return collection;
}

static void
_TestSerialization(
    const std::vector<std::shared_ptr<TraceCollection>>& testCols,
    std::string fileName)
{
    std::stringstream test;
    bool written = false;
    if (testCols.size() == 1 ) {
        written = TraceSerialization::Write(test, testCols[0]);
    } else {
        written = TraceSerialization::Write(test, testCols);
    }
    TF_AXIOM(written);

    // Write out the file
    {
        std::ofstream ostream(fileName);
        ostream << test.str();
    }
    // Read a collection from the file just written
    std::shared_ptr<TraceCollection> collection;
    {
        std::ifstream istream(fileName);
        collection = TraceSerialization::Read(istream);
        TF_AXIOM(collection);
    }

    std::string stringRepr = test.str();

    std::stringstream test2;
    bool written2 = TraceSerialization::Write(test2, collection);
    TF_AXIOM(written2);

    std::string stringRepr2 = test2.str();

    // This comparison might be too strict.
    if (stringRepr != stringRepr2) {
        std::cout << "Written:\n" << stringRepr << "\n";
        std::cout << "Reconstruction:\n" << stringRepr2 << "\n";
    }
    TF_AXIOM(stringRepr == stringRepr2);
}

int
main(int argc, char *argv[]) 
{
    TraceCategory::GetInstance().RegisterCategory(
        TestCategory, "Test Category");

    std::vector<std::shared_ptr<TraceCollection>> collections;
    std::cout << "Testing single collection\n";
    collections.emplace_back(CreateTestCollection());
    _TestSerialization(collections, "trace.json");
    std::cout << " PASSED\n";

    std::cout << "Testing multiple collections\n";
    collections.emplace_back(CreateTestCollection(20.0/1000.0));
    _TestSerialization(collections, "trace2.json");
    std::cout << " PASSED\n";
}

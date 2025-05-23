# Copyright 2018 Pixar
#
# Licensed under the terms set forth in the LICENSE.txt file available at
# https://openusd.org/license.
#
# Modified by Jeremy Retailleau.

from __future__ import print_function

import sys
import os
import time
import unittest

from pxr import Trace, Tf


# Helper functions
def TraverseTraceTree(root):
    nodes = [root]
    while nodes:
        node = nodes.pop()
        nodes.extend(node.children)
        yield node

def GetNodesByKey(reporter, key):
    def getNodesRecur(node, key):
        result = []
        if node.key == key:
            result.append(node)
        for child in node.children:
            result.extend(getNodesRecur(child, key))
        return result
    return getNodesRecur(reporter.aggregateTreeRoot, key)

class TestTrace(unittest.TestCase):
    def test_Trace(self):
        gc = Trace.Collector()
        self.assertIsInstance(gc, Trace.Collector)

        self.assertEqual(gc, Trace.Collector())

        gr = Trace.Reporter.globalReporter
        self.assertIsInstance(gr, Trace.Reporter)
        gr.shouldAdjustForOverheadAndNoise = False

        if not os.getenv('PXR_ENABLE_GLOBAL_TRACE', False):
            self.assertEqual(gc.enabled , False)
            self.assertEqual(gc.EndEvent('Begin') , 0)
            gc.enabled = True
        else:
            self.assertEqual(gc.enabled , True)

        # begin and end.
        gc.BeginEvent('Begin')
        gc.BeginEvent('Begin')
        gc.BeginEvent('Begin')

        gc.EndEvent('Begin')
        gc.EndEvent('Begin')
        gc.EndEvent('Begin')


        gc.enabled = False

        # this shouldn't add any events

        gc.BeginEvent('Begin')
        gc.EndEvent('Begin')

        gc.enabled = True
        gr.UpdateTraceTrees()

        beginNodes = GetNodesByKey(gr, 'Begin')
        print(len(beginNodes))
        self.assertEqual(len(beginNodes) , 3)

        for eventNode in beginNodes:
            self.assertEqual(eventNode.key , 'Begin')
            self.assertTrue(eventNode.exclusiveTime > 0 and \
                eventNode.inclusiveTime > 0)

        gc.Clear()
        gr.ClearTree()

        self.assertEqual(len(GetNodesByKey(gr, 'Begin')) , 0)


        ## The following is more example usage than a test....  It is left in to
        ## show usage and to exercise the python decorators.

        class Test:
            @Trace.TraceMethod
            def __init__(self, arg):
                self.__arg = arg
            @Trace.TraceMethod
            def Func(self):
                pass

        @Trace.TraceFunction
        def testFunc2():
            t = Test(1.234)
            t.Func()

        @Trace.TraceFunction
        def testFunc():
            gc = Trace.Collector()

            gc.BeginEvent('Custom Paired Event')

            testFunc2()
                
            gc.EndEvent('Custom Paired Event')

        testFunc()


        gr.ClearTree()
        Trace.TestAuto()
        gr.UpdateTraceTrees()

        # Should have generated a top-level event
        autoNodes = GetNodesByKey(gr, 'TestAuto')
        self.assertEqual(len(autoNodes), 1)

        self.assertEqual(autoNodes[0].key , 'TestAuto')

        self.assertTrue(autoNodes[0].exclusiveTime > 0)
        self.assertTrue(autoNodes[0].inclusiveTime > 0)

        gr.ClearTree()
        Trace.TestNesting()
        gr.Report()

        # Should have generated a top-level event
        nestingNodes = GetNodesByKey(gr, 'TestNesting')
        self.assertEqual(len(nestingNodes), 1)
        self.assertEqual(nestingNodes[0].key , 'TestNesting')
        self.assertTrue(nestingNodes[0].exclusiveTime > 0)
        self.assertTrue(nestingNodes[0].inclusiveTime > 0)


        Trace.TestNesting()
        gr.UpdateTraceTrees()
        rootNode = gr.aggregateTreeRoot
        # code cover and check some of the exposed parts of EventNode
        for child in rootNode.children :
            print("inc: ", "%.3f" % child.inclusiveTime) 
            print("exc: ", "%.3f" % child.exclusiveTime)
            print("cnt: ", "%d" % child.count)
            child.expanded = True
            self.assertTrue(child.expanded)
            
        gr.Report(len(rootNode.children))
        gr.ClearTree()
        gc.Clear()

        Trace.TestCreateEvents()
        pythonEvent = 'PYTHON_EVENT'
        gc.BeginEvent(pythonEvent)
        gc.EndEvent(pythonEvent)

        gr.UpdateTraceTrees()
        self.assertEqual(len(GetNodesByKey(gr, Trace.GetTestEventName())), 1)

        gr.ReportTimes()

        gc.BeginEvent('Foo')
        gc.BeginEvent('Bar')
        gc.BeginEvent('Foo')

        gc.EndEvent('Foo')
        gc.EndEvent('Bar')
        gc.EndEvent('Foo')

        gr.Report()

        gc.enabled = False
        gr.Report()

        self.assertEqual(gc.EndEvent("BadEvent"), 0)
        gc.enabled = True

        # Also try a local reporter and scope
        lr = Trace.Reporter('local reporter')
        self.assertEqual(lr.GetLabel() , 'local reporter')

        gc.BeginEvent("LocalScope")
        gc.EndEvent("LocalScope")

        gc.enabled = False

        lr.Report()

        gc.enabled = True
        sleepTime = 1.0
        pre_begin = time.time()
        b = gc.BeginEvent("Test tuple")
        post_begin = time.time()
        time.sleep(sleepTime)
        pre_end = time.time()
        e = gc.EndEvent("Test tuple")
        post_end = time.time()

        elapsedSeconds = Trace.GetElapsedSeconds(b, e)
        expectedMinSeconds = pre_end - post_begin
        expectedMaxSeconds = post_end - pre_begin
        gr.Report()

        epsilon = 0.005
        self.assertGreater(elapsedSeconds, expectedMinSeconds - epsilon,
                        "Elapsed: {} Expected Min: {} Diff: {}".format(
                            elapsedSeconds, expectedMinSeconds,
                            expectedMinSeconds - elapsedSeconds))
        self.assertLess(elapsedSeconds, expectedMaxSeconds + epsilon,
                        "Elapsed: {} Expected Max: {} Diff: {}".format(
                            elapsedSeconds, expectedMaxSeconds,
                            elapsedSeconds - expectedMaxSeconds))
        print("")

    def test_TracePythonGarbageCollection(self):
        '''Test that trace scopes are generated during Python garbage
        collection when the gc callback is installed and collection is enabled.
        '''

        def HasGarbageCollectionNode(reporter):
            '''Returns true if the reporter has one or more nodes that look
            like they were generated by Trace.PythonGarbageCollectionCallback.
            '''
            reporter.UpdateTraceTrees()
            return any(
                n.key.startswith("Python Garbage Collection")
                for n in TraverseTraceTree(reporter.aggregateTreeRoot))

        import gc
        collector = Trace.Collector()
        reporter = Trace.Reporter.globalReporter

        collector.enabled = False

        collector.Clear()
        reporter.ClearTree()

        gc.callbacks.append(Trace.PythonGarbageCollectionCallback)
        try:
            collector.enabled = True
            gc.collect()
            collector.enabled = False

            reporter.UpdateTraceTrees()
            self.assertTrue(HasGarbageCollectionNode(reporter))

            collector.Clear()
            reporter.ClearTree()

            gc.collect()

            reporter.UpdateTraceTrees()
            self.assertFalse(HasGarbageCollectionNode(reporter))
        finally:
            gc.callbacks.remove(Trace.PythonGarbageCollectionCallback)

if __name__ == '__main__':
    unittest.main()


if (WIN32)
    # Patch CMake's 'add_test' to extend the DLL search path on Windows. Use the
    # target name associated with the test, if it is also a CMake target.

    macro(add_test)
        _add_test(${ARGV})

        cmake_parse_arguments("" "" "NAME" "" ${ARGV})
        if (TARGET ${_NAME})
            set(DLL_DIRS $<TARGET_RUNTIME_DLL_DIRS:${_NAME}>)
            set_tests_properties(${_NAME}
                PROPERTIES ENVIRONMENT
                "PATH=$<JOIN:$<SHELL_PATH:${DLL_DIRS}>,\\;>")
        endif()
    endmacro()
endif()

add_executable(testTraceCategory testTraceCategory.cpp)
target_link_libraries(testTraceCategory PUBLIC trace)
add_test(NAME testTraceCategory COMMAND testTraceCategory)

add_executable(testTraceData testTraceData.cpp)
target_link_libraries(testTraceData PUBLIC trace)
add_test(NAME testTraceData COMMAND testTraceData)

add_executable(testTraceHash testTraceHash.cpp)
target_link_libraries(testTraceHash PUBLIC trace)
add_test(NAME testTraceHash COMMAND testTraceHash)

add_executable(testTraceMacros testTraceMacros.cpp)
target_link_libraries(testTraceMacros PUBLIC trace)
add_test(NAME testTraceMacros COMMAND testTraceMacros)

add_executable(testTraceOverhead testTraceOverhead.cpp)
target_link_libraries(testTraceOverhead PUBLIC trace)
add_test(NAME testTraceOverhead COMMAND testTraceOverhead)

add_executable(testTraceThreading testTraceThreading.cpp)
target_link_libraries(testTraceThreading PUBLIC trace)
add_test(NAME testTraceThreading COMMAND testTraceThreading)

if(BUILD_PYTHON_BINDINGS)
    set(testWrapper
        "${Python_EXECUTABLE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/testWrapper.py" --verbose
        "--python-path=${CMAKE_CURRENT_SOURCE_DIR}"
        "--python-path=$<TARGET_FILE_DIR:pyTrace>/../.."
        "--python-path=$<TARGET_FILE_DIR:pxr::pyTf>/../.."
        "--library-path=$<TARGET_FILE_DIR:trace>"
        "--library-path=$<TARGET_FILE_DIR:pyTrace>"
        "--library-path=$<TARGET_FILE_DIR:pxr::tf>"
        "--library-path=$<TARGET_FILE_DIR:pxr::pyTf>")

    add_test(NAME testTrace
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTrace.py")

    add_test(NAME testTraceRecursion1
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_5begins.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceRecursion2
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_typical.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceRecursion3
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_newnode.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceRecursion4
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_inner.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceRecursion5
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_1817.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceRecursion6
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_branch.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceRecursion7
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.py"
        --diff-compare=recursion_marker_merge.out
        --baseline-dir=${CMAKE_CURRENT_SOURCE_DIR}/testTraceRecursion.testenv/baseline)

    add_test(NAME testTraceReports
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceReports.py")

    add_test(NAME testTraceReporter
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceReporter.py")

    add_test(NAME testTraceReporterLoadTrace
        COMMAND ${testWrapper}
        "${Python_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/testTraceReporterLoadTrace.py")
endif()

add_executable(testTraceSerialization testTraceSerialization.cpp)
target_link_libraries(testTraceSerialization PUBLIC trace)
add_test(NAME testTraceSerialization COMMAND testTraceSerialization)

add_executable(testTraceCounters testTraceCounters.cpp)
target_link_libraries(testTraceCounters PUBLIC trace)
add_test(NAME testTraceCounters COMMAND testTraceCounters)

add_executable(testTraceMarkers testTraceMarkers.cpp)
target_link_libraries(testTraceMarkers PUBLIC trace)
add_test(NAME testTraceMarkers COMMAND testTraceMarkers)

add_executable(testTraceReportPerf testTraceReportPerf.cpp)
target_link_libraries(testTraceReportPerf PUBLIC trace)
add_test(NAME testTraceReportPerf COMMAND testTraceReportPerf)

add_executable(testTraceEventContainer testTraceEventContainer.cpp)
target_link_libraries(testTraceEventContainer PUBLIC trace)
add_test(NAME testTraceEventContainer COMMAND testTraceEventContainer)

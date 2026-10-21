cmake_minimum_required(VERSION 3.24)
project(WarfareFrontNative LANGUAGES CXX)

# This builds real, engine-independent rule/scene code. It does not replace UE.
set(WF_FRRU "${CMAKE_CURRENT_SOURCE_DIR}/Source/WarfareFront/Private/FRRU")
set(WF_NATIVE "${WF_FRRU}/Native")
add_library(warfare_front_native STATIC
    "${WF_NATIVE}/Expansion.cpp"
    "${WF_NATIVE}/SceneRuntime.cpp"
    "${WF_NATIVE}/TacticalRuntime.cpp")
target_include_directories(warfare_front_native PUBLIC "${WF_FRRU}" "${WF_NATIVE}")
target_compile_features(warfare_front_native PUBLIC cxx_std_20)
if(MSVC)
    target_compile_options(warfare_front_native PUBLIC /utf-8)
endif()

include(CTest)
if(BUILD_TESTING)
    add_executable(wf_native_orders tests/native/native_orders_v826_tests.cpp)
    target_link_libraries(wf_native_orders PRIVATE warfare_front_native)
    add_test(NAME native_orders COMMAND wf_native_orders "${CMAKE_CURRENT_BINARY_DIR}/native-orders.json")

    add_executable(wf_native_triangles tests/native/native_triangle_traversal_v87_tests.cpp)
    target_link_libraries(wf_native_triangles PRIVATE warfare_front_native)
    add_test(NAME native_triangles COMMAND wf_native_triangles "${CMAKE_CURRENT_BINARY_DIR}/native-triangles.json")
endif()

#----------------------------------------------------------------------
#----------------------------------------------------------------------
cmake_minimum_required(VERSION 3.5.1)

project(grpc-perf C CXX)

option(INCLUDE_GRPC_SIDEBAND "Include support for profiling gprc sideband tests" OFF)
option(INCLUDE_FLAATBUFFERS "Include support for profiling Flatbuffers" OFF)
option(INCLUDE_UDS_TESTS "Include support for testing performance over UDS (Unix Domain Sockets)" OFF)

if(NOT MSVC)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--allow-shlib-undefined")
else()
  cmake_policy(SET CMP0091 NEW)
  add_definitions(-D_WIN32_WINNT=0x600 -bigobj)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4267 /wd4244 /wd4018")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

if (INCLUDE_GRPC_SIDEBAND)
  add_definitions(-DENABLE_GRPC_SIDEBAND)
endif()

if (INCLUDE_FLAATBUFFERS)
add_definitions(-DENABLE_FLATBUFFERS)
endif()

if (INCLUDE_UDS_TESTS)
add_definitions(-DENABLE_UDS_TESTS)
endif()

find_package(Threads REQUIRED)
link_directories(.)

#----------------------------------------------------------------------
# Include the gRPC's cmake build
#----------------------------------------------------------------------
add_subdirectory(third_party/grpc ${CMAKE_CURRENT_BINARY_DIR}/grpc EXCLUDE_FROM_ALL)

#----------------------------------------------------------------------
# Include the grpc-sideband cmake build
#----------------------------------------------------------------------
if (INCLUDE_GRPC_SIDEBAND)
  add_subdirectory(third_party/grpc-sideband ${CMAKE_CURRENT_BINARY_DIR}/grpc-sideband)
endif()

#----------------------------------------------------------------------
# Use the grpc targets directly from this build.
#----------------------------------------------------------------------
set(_PROTOBUF_LIBPROTOBUF libprotobuf)
set(_REFLECTION grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
else()
  set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()
set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:grpc_cpp_plugin>)
endif()

#----------------------------------------------------------------------
# Define libraries to link
#----------------------------------------------------------------------
if (INCLUDE_GRPC_SIDEBAND)
  if(WIN32)
    link_directories("./third_party/grpc-sideband")
  else()
    # Add grpc-sideband deps here
    link_directories("./third_party/grpc-sideband/linux")
  endif()
endif()

#----------------------------------------------------------------------
# Proto file
#----------------------------------------------------------------------
get_filename_component(datamoniker_proto "data_moniker.proto" ABSOLUTE)
get_filename_component(datamoniker_proto_path "${datamoniker_proto}" PATH)

#----------------------------------------------------------------------
# Generated data moniker sources
#----------------------------------------------------------------------
set(datamoniker_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/data_moniker.pb.cc")
set(datamoniker_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/data_moniker.pb.h")
set(datamoniker_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/data_moniker.grpc.pb.cc")
set(datamoniker_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/data_moniker.grpc.pb.h")
add_custom_command(
  OUTPUT "${datamoniker_proto_srcs}" "${datamoniker_proto_hdrs}" "${datamoniker_grpc_srcs}" "${datamoniker_grpc_hdrs}"
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${datamoniker_proto_path}" -I "${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${datamoniker_proto}"
  DEPENDS "${datamoniker_proto}")

#----------------------------------------------------------------------
# Proto file
#----------------------------------------------------------------------
get_filename_component(perftest_proto "perftest.proto" ABSOLUTE)
get_filename_component(perftest_proto_path "${perftest_proto}" PATH)

#----------------------------------------------------------------------
# Generated sources
#----------------------------------------------------------------------
set(perftest_proto_srcs "${CMAKE_CURRENT_BINARY_DIR}/perftest.pb.cc")
set(perftest_proto_hdrs "${CMAKE_CURRENT_BINARY_DIR}/perftest.pb.h")
set(perftest_grpc_srcs "${CMAKE_CURRENT_BINARY_DIR}/perftest.grpc.pb.cc")
set(perftest_grpc_hdrs "${CMAKE_CURRENT_BINARY_DIR}/perftest.grpc.pb.h")
add_custom_command(
  OUTPUT "${perftest_proto_srcs}" "${perftest_proto_hdrs}" "${perftest_grpc_srcs}" "${perftest_grpc_hdrs}"
  COMMAND ${_PROTOBUF_PROTOC}
  ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
    --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
    -I "${perftest_proto_path}" -I "${CMAKE_CURRENT_SOURCE_DIR}/third_party/grpc/third_party/protobuf/src"
    --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
    "${perftest_proto}"
  DEPENDS "${perftest_proto}")

#----------------------------------------------------------------------
# Include generated *.pb.h files
#----------------------------------------------------------------------
include_directories("${CMAKE_CURRENT_BINARY_DIR}" "./src" "./third_party/grpc" "./third_party/grpc-sideband/src" "/snap/flatbuffers/current/include")

#----------------------------------------------------------------------
# perftest gRPC Server
#----------------------------------------------------------------------
add_executable(perftest_server
  src/perftest_server.cc
  src/perftest_server_app.cc
  src/client_utilities.cc
  src/performance_tests.cc
  ${perftest_proto_srcs}
  ${perftest_grpc_srcs}
  ${datamoniker_proto_srcs}
  ${datamoniker_grpc_srcs}
  )
if (INCLUDE_GRPC_SIDEBAND)
  target_link_libraries(perftest_server
    ni_grpc_sideband
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    ${DETOURS_LIB}
    )
else()
  target_link_libraries(perftest_server
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    ${DETOURS_LIB}
    )
endif()

#----------------------------------------------------------------------
# perftest gRPC Server
#----------------------------------------------------------------------
add_executable(local_perf_check
  src/perftest_server.cc
  src/localhost_perf_check.cc
  src/client_utilities.cc
  src/performance_tests.cc
  ${perftest_proto_srcs}
  ${perftest_grpc_srcs}
  ${datamoniker_proto_srcs}
  ${datamoniker_grpc_srcs}
  )
if (INCLUDE_GRPC_SIDEBAND)
  target_link_libraries(local_perf_check
    ni_grpc_sideband
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    ${DETOURS_LIB}
    )
else()
  target_link_libraries(local_perf_check
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    ${DETOURS_LIB}
    )
endif()

#----------------------------------------------------------------------
# Example C++ application to talk to the perftest gRPC service
#----------------------------------------------------------------------
add_executable(perftest_client
  src/perftest_client.cc
  src/client_utilities.cc
  src/performance_tests.cc
  ${perftest_proto_srcs}
  ${perftest_grpc_srcs}
  ${datamoniker_proto_srcs}
  ${datamoniker_grpc_srcs}
  )
if (INCLUDE_GRPC_SIDEBAND)
  target_link_libraries(perftest_client
    ni_grpc_sideband
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    )
else()
  target_link_libraries(perftest_client
    ${_REFLECTION}
    ${_GRPC_GRPCPP}
    ${_PROTOBUF_LIBPROTOBUF}
    )
endif()

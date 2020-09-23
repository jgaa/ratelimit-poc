cmake_minimum_required (VERSION 3.14)

project (rated VERSION 0.1.4 LANGUAGES CXX)

set(RATED_VERSION "${CMAKE_PROJECT_VERSION}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Protobuf REQUIRED)
find_package(gRPC REQUIRED)
    
set(PROTOS
    ${CMAKE_CURRENT_SOURCE_DIR}/proto/ratelimit.proto
)

set(PROTO_SRC_DIR ${CMAKE_CURRENT_BINARY_DIR}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_SRC_DIR} ${PROTOS})
grpc_generate_cpp(GRPC_SRCS GRPC_HDRS ${PROTO_SRC_DIR} ${PROTOS})


add_executable(${PROJECT_NAME}
    main.cpp
    RateLimitSvc.cpp
    RateLimitSvc.h
    RateLimitSvcImpl.cpp
    RateLimiter.cpp
    RateLimiter.h
    ${PROTO_SRCS}
    ${GRPC_SRCS}
)

add_definitions(
    -DRATED_VERSION=${RATED_VERSION}
)


set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED YES
    CXX_EXTENSIONS NO
)

target_link_libraries(${PROJECT_NAME}
    grpc++
    protobuf::libprotobuf
    grpc
)


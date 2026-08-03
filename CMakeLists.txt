cmake_minimum_required(VERSION 3.16)
project(remarkable_chess VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(chess_core
    src/chess_state.cpp
    src/chess_session.cpp
)
target_include_directories(chess_core PUBLIC include)

add_executable(remarkable_chess_backend
    src/appload_backend.cpp
)
target_link_libraries(remarkable_chess_backend PRIVATE chess_core)

include(CTest)
if(BUILD_TESTING)
    add_executable(test_chess_state tests/test_chess_state.cpp)
    target_link_libraries(test_chess_state PRIVATE chess_core)
    add_test(NAME chess_state COMMAND test_chess_state)

    add_executable(test_chess_session tests/test_chess_session.cpp)
    target_link_libraries(test_chess_session PRIVATE chess_core)
    add_test(NAME chess_session COMMAND test_chess_session)
endif()

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

option(BUILD_REMARKABLE_APP "Build the Qt Quick application" ON)
if(BUILD_REMARKABLE_APP)
    find_package(Qt6 6.5 REQUIRED COMPONENTS Quick)
    qt_standard_project_setup(REQUIRES 6.5)

    qt_add_executable(remarkable_chess
        src/main.cpp
        src/chess_controller.cpp
        include/chess_controller.h
    )
    qt_add_qml_module(remarkable_chess
        URI Omi.RemarkableChess
        VERSION 1.0
        QML_FILES qml/Main.qml
    )
    target_link_libraries(remarkable_chess PRIVATE chess_core Qt6::Quick)
endif()

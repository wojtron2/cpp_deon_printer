cmake_minimum_required(VERSION 3.16)

project(deon_printer_app LANGUAGES CXX)

# ustawienia standardu c++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# wspolna biblioteka z logika aplikacji
add_library(deon_lib
    src/money_utils.cpp
    src/novitus_protocol.cpp
    src/receipt_logic.cpp
    src/serial_port.cpp
)

# naglowki sa w katalogu src
target_include_directories(deon_lib
    PUBLIC
        src
)

# glowna aplikacja
add_executable(deon_printer_app
    src/main.cpp
)

target_link_libraries(deon_printer_app
    PRIVATE
        deon_lib
)

# wlaczenie obslugi testow
enable_testing()

# lista testow odpowiadajaca plikom tests/test_*.cpp
set(TESTS
    money_utils
    novitus_protocol
    receipt_logic
)

# tworzenie targetow testowych w petli
set(TEST_TARGETS "")

foreach(t ${TESTS})
    add_executable(test_${t} tests/test_${t}.cpp)
    target_link_libraries(test_${t} PRIVATE deon_lib)
    add_test(NAME test_${t} COMMAND test_${t})
    list(APPEND TEST_TARGETS test_${t})
endforeach()

# docelowy target uruchamiajacy wszystkie testy z pelnym outputem
add_custom_target(run_tests
    COMMAND ${CMAKE_CTEST_COMMAND} -V
    DEPENDS ${TEST_TARGETS}
)

cmake_minimum_required(VERSION 3.16)
project(kis VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

find_package(PkgConfig REQUIRED)
pkg_check_modules(X11 REQUIRED x11)
pkg_check_modules(SQLite3 REQUIRED sqlite3)
find_package(OpenSSL REQUIRED)
find_package(GTest REQUIRED)

add_executable(kis src/main.cpp src/input_stats.cpp src/storage.cpp src/encryptor.cpp)
target_include_directories(kis PRIVATE src ${X11_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})
target_link_libraries(kis PRIVATE ${X11_LIBRARIES} ${SQLite3_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)

enable_testing()
add_executable(test_kis tests/test_kis.cpp src/input_stats.cpp src/storage.cpp src/encryptor.cpp)
target_include_directories(test_kis PRIVATE src ${X11_INCLUDE_DIRS} ${SQLite3_INCLUDE_DIRS})
target_link_libraries(test_kis PRIVATE GTest::GTest GTest::MainMock ${X11_LIBRARIES} ${SQLite3_LIBRARIES} OpenSSL::SSL OpenSSL::Crypto)

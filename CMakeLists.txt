cmake_minimum_required(VERSION 3.10.0)

set(CMAKE_TOOLCHAIN_FILE "$ENV{VCPKG_ROOT}/scripts/buildsystems/vcpkg.cmake" CACHE STRING "")
project(main VERSION 0.1.0 LANGUAGES C CXX)


add_executable(main 
    main.cpp
    md5_util.cpp
    DBConnection.cpp
    Logger.cpp
)


find_package(unofficial-libmysql REQUIRED)
target_link_libraries(main PRIVATE unofficial::libmysql::libmysql)

find_package(OpenSSL REQUIRED)
target_link_libraries(main PRIVATE OpenSSL::SSL)
target_link_libraries(main PRIVATE OpenSSL::Crypto)


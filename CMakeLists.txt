cmake_minimum_required(VERSION 3.10)
project(EduDesk LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Widgets Sql Core REQUIRED)
find_package(OpenSSL REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

include_directories(${CMAKE_SOURCE_DIR}/src)

if (OPENSSL_INCLUDE_DIR)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

include_directories(${SODIUM_INCLUDE_DIRS})

file(GLOB_RECURSE ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*/*.cpp"
)

set(SRC_FILES "")
foreach(_f ${ALL_SRC})
    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_f}")
    if (NOT _rel MATCHES "^src/tools/")
        list(APPEND SRC_FILES "${_f}")
    endif()
endforeach()

add_executable(EduDesk
    ${SRC_FILES}
)

target_link_libraries(EduDesk
    Qt5::Widgets
    Qt5::Sql
    Qt5::Core
    ${SODIUM_LIBRARIES}
)

if (TARGET OpenSSL::Crypto)
    target_link_libraries(EduDesk OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(EduDesk ${OPENSSL_LIBRARIES})
endif()

add_executable(create_admin
    src/tools/create_admin.cpp
    src/db/Database.cpp
    src/config/ConfigManager.cpp
    src/auth/PasswordUtils.cpp
    src/auth/AuthManager.cpp
)

target_link_libraries(create_admin
    Qt5::Core
    Qt5::Sql
    ${SODIUM_LIBRARIES}
)

if (TARGET OpenSSL::Crypto)
    target_link_libraries(create_admin OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(create_admin ${OPENSSL_LIBRARIES})
endif()

add_executable(create_submission
    src/tools/create_submission.cpp
    src/db/Database.cpp
    src/config/ConfigManager.cpp
    src/crypto/FileCrypto.cpp
    src/crypto/KeyProtect.cpp
)

target_link_libraries(create_submission
    Qt5::Core
    Qt5::Sql
    ${SODIUM_LIBRARIES}
)

if (TARGET OpenSSL::Crypto)
    target_link_libraries(create_submission OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(create_submission ${OPENSSL_LIBRARIES})
endif()

add_custom_target(tools ALL
    DEPENDS create_admin create_submission
)

if (UNIX)
    set_target_properties(EduDesk PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    set_target_properties(create_admin PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    set_target_properties(create_submission PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

message(STATUS "Project configured. Sources for EduDesk: ${SRC_FILES}")

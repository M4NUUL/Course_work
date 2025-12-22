cmake_minimum_required(VERSION 3.10)
project(Jumandgi LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Автоматический moc / uic / rcc для Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Найдём Qt5
find_package(Qt5 COMPONENTS Widgets Sql Core REQUIRED)

# Найдём OpenSSL
find_package(OpenSSL REQUIRED)

# Указываем include для нашего исходного дерева
include_directories(${CMAKE_SOURCE_DIR}/src)
if (OPENSSL_INCLUDE_DIR)
    include_directories(${OPENSSL_INCLUDE_DIR})
endif()

# Собираем список всех .cpp в src, но исключаем src/tools/*
file(GLOB_RECURSE ALL_SRC
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*/*.cpp"
)

set(SRC_FILES "")
foreach(_f ${ALL_SRC})
    # делаем путь относительно корня, чтобы проще проверять
    file(RELATIVE_PATH _rel "${CMAKE_SOURCE_DIR}" "${_f}")
    if (NOT _rel MATCHES "^src/tools/")
        list(APPEND SRC_FILES "${_f}")
    endif()
endforeach()

# Основной исполняемый файл (GUI)
add_executable(Jumandgi
    ${SRC_FILES}
)

# Линкуем Jumandgi с Qt и OpenSSL (аккуратно: OpenSSL::Crypto/SSL если доступны)
target_link_libraries(Jumandgi
    Qt5::Widgets
    Qt5::Sql
    Qt5::Core
)

if (TARGET OpenSSL::Crypto)
    target_link_libraries(Jumandgi OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(Jumandgi ${OPENSSL_LIBRARIES})
endif()

# --- Утилита create_admin (собирается отдельно) ---
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
)

if (TARGET OpenSSL::Crypto)
    target_link_libraries(create_admin OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(create_admin ${OPENSSL_LIBRARIES})
endif()

# --- Утилита create_submission (собирается отдельно) ---
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
)

if (TARGET OpenSSL::Crypto)
    target_link_libraries(create_submission OpenSSL::Crypto OpenSSL::SSL)
else()
    target_link_libraries(create_submission ${OPENSSL_LIBRARIES})
endif()

# Удобная цель для сборки всех утилит (опционально)
add_custom_target(tools ALL
    DEPENDS create_admin create_submission
)

# Устанавливаем rpaths корректно для разработки (не обязательно)
if (UNIX)
    set_target_properties(Jumandgi PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    set_target_properties(create_admin PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
    set_target_properties(create_submission PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)
endif()

message(STATUS "Project configured. Sources for Jumandgi: ${SRC_FILES}")

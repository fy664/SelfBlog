# =============================================================
# 全栈个人博客系统 - CMake 构建配置
# 平台：Windows(MSVC) / Linux(Ubuntu, g++)
# =============================================================
cmake_minimum_required(VERSION 3.16)
project(blog_server CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---- 第三方单头文件（httplib / nlohmann-json）----
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/third_party)

# ---- 源文件 ----
file(GLOB BLOG_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/util/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/store/*.cpp
)

add_executable(blog_server ${BLOG_SRC})

if(WIN32)
  # cpp-httplib 依赖 Winsock
  target_link_libraries(blog_server ws2_32)
  if(MSVC)
    # /utf-8 保证源码中文字符串在 MSVC 下按 UTF-8 编译
    # /EHsc 启用 C++ 异常；/W3 常规警告级别
    target_compile_options(blog_server PRIVATE /utf-8 /EHsc /W3 /wd4996)
  endif()

  # ---- MySQL 开发库探测（Windows 本地：D:\MySQL8 或标准安装目录）----
  find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS "D:/MySQL8/include" "C:/Program Files/MySQL/MySQL Server 8.0/include")
  find_library(MYSQL_LIBRARY NAMES libmysql
    PATHS "D:/MySQL8/lib" "C:/Program Files/MySQL/MySQL Server 8.0/lib")

  if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    message(STATUS "MySQL 开发库已找到（${MYSQL_LIBRARY}），启用 MySqlStore")
    target_include_directories(blog_server PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(blog_server ${MYSQL_LIBRARY})
    target_compile_definitions(blog_server PRIVATE HAVE_MYSQL=1)
    # 运行时依赖 libmysql.dll：构建后自动拷贝到产物目录
    get_filename_component(MYSQL_BIN_DIR ${MYSQL_LIBRARY} DIRECTORY)
    add_custom_command(TARGET blog_server POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different
              "${MYSQL_BIN_DIR}/libmysql.dll" "$<TARGET_FILE_DIR:blog_server>/libmysql.dll")
    # libmysql.dll 还依赖 OpenSSL（libssl/libcrypto），一并拷贝
    foreach(_dep libssl-3-x64.dll libcrypto-3-x64.dll zlib.dll)
      if(EXISTS "${MYSQL_BIN_DIR}/${_dep}")
        add_custom_command(TARGET blog_server POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different
                  "${MYSQL_BIN_DIR}/${_dep}" "$<TARGET_FILE_DIR:blog_server>/${_dep}")
      endif()
    endforeach()
  else()
    message(STATUS "未找到 MySQL 开发库，仅编译 JsonStore（本地开发模式）")
  endif()
else()
  find_package(Threads REQUIRED)
  target_link_libraries(blog_server Threads::Threads)

  # ---- MySQL 开发库探测（Linux 部署环境）----
  # Ubuntu：sudo apt install -y libmysqlclient-dev
  find_path(MYSQL_INCLUDE_DIR mysql.h
    PATHS /usr/include/mysql /usr/local/include/mysql)
  find_library(MYSQL_LIBRARY NAMES mysqlclient
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib64 /usr/lib)

  if(MYSQL_INCLUDE_DIR AND MYSQL_LIBRARY)
    message(STATUS "MySQL 开发库已找到，启用 MySqlStore")
    target_include_directories(blog_server PRIVATE ${MYSQL_INCLUDE_DIR})
    target_link_libraries(blog_server ${MYSQL_LIBRARY})
    target_compile_definitions(blog_server PRIVATE HAVE_MYSQL=1)
  else()
    message(STATUS "未找到 MySQL 开发库，仅编译 JsonStore（本地开发模式）")
  endif()
endif()

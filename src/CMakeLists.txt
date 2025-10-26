cmake_minimum_required(VERSION 3.16)
project(thenews LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_CURRENT_SOURCE_DIR}/pages)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 COMPONENTS Widgets REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBNOTIFY REQUIRED libnotify)
pkg_check_modules(GLIB REQUIRED glib-2.0)

add_executable(thenews 
    main.cpp
    pages/page1.ui
    pages/page2.ui
    resources.qrc
)
target_include_directories(thenews PRIVATE ${LIBNOTIFY_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS})
target_link_libraries(thenews PRIVATE Qt6::Widgets ${LIBNOTIFY_LIBRARIES} ${GLIB_LIBRARIES})

cmake_minimum_required(VERSION 3.16)
project(quasar LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(quasar
    src/main.cpp
    src/main_window.cpp
    src/title_bar.cpp
    src/cosmic_background.cpp
    src/spreadsheet_model.cpp
    src/spreadsheet_view.cpp
    src/sheet.cpp
    src/formula_engine.cpp
    src/toolbar.cpp
)
target_include_directories(quasar PRIVATE src)
target_link_libraries(quasar PRIVATE Qt6::Widgets)
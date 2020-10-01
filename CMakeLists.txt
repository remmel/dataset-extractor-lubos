cmake_minimum_required(VERSION 3.0.2)

find_library(turbojpeg-lib turbojpeg)
find_library(png-lib png)

include_directories(
    common
)

add_executable(
    dataset_extractor
    common/data/file3d.cc
    common/data/image.cc
    common/data/mesh.cc
    app.cpp
)

target_link_libraries(
    dataset_extractor
    png
    turbojpeg
)

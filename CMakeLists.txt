cmake_minimum_required (VERSION 3.8)

project("coco-bonus")

add_executable(coco-bonus "main.cpp" "refcount.h" "refcount_managed.h")

set_property(TARGET coco-bonus PROPERTY CXX_STANDARD 20)

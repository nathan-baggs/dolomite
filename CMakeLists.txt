cmake_minimum_required(VERSION 3.22)

SET(CMAKE_EXPORT_COMPILE_COMMANDS ON)

project(
	dolomite
	VERSION "0.0.1"
	DESCRIPTION "nightfire hacking"
	LANGUAGES C CXX
)

add_library(ddraw SHARED
	src/ddraw.cpp
	src/ddraw.def
)
set_property(TARGET ddraw PROPERTY
  MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
target_compile_features(ddraw PUBLIC cxx_std_23)
target_compile_options(ddraw PUBLIC /W3 /WX)


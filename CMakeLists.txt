#########################################################################################
#
# CMakeLists.txt for CME (Coastal Modelling Environment)
#
#########################################################################################
#set(CMAKE_LEGACY_CYGWIN_WIN32 0) # Remove when CMake >= 2.8.4 is required
message("Creating Makefile for CME")
message("")

message("Checking compiler")
cmake_minimum_required(VERSION 2.6)
enable_language(CXX)
#set(FIND_LIBRARY_USE_LIB64_PATHS true)

project(CME)
file(GLOB CME_SOURCE_FILES *.cpp)
set(CME_EXECUTABLE cme)
set(CME_INSTALL_DIR "$ENV{HOME}/coast/iCOASST")
set(CMAKE_PREFIX_PATH "$ENV{HOME}")

#
# If build type not specified, assume a Debug build
#
if(NOT CMAKE_BUILD_TYPE)
   set(CMAKE_BUILD_TYPE Debug)
endif(NOT CMAKE_BUILD_TYPE)

#
# Set up the compiler flags and build directories for the various build types
#
if(CMAKE_BUILD_TYPE MATCHES Debug)
   set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_DEBUG})
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CME_BINARY_DIR}/Debug CACHE PATH "Directory for Debug builds" FORCE)
   # For Debug builds on any platform, add the model's "internal" debug flag
   # (used for extra logging) etc.
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_DEBUG")
endif(CMAKE_BUILD_TYPE MATCHES Debug)

if(CMAKE_BUILD_TYPE MATCHES Release)
   set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELEASE})
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CME_BINARY_DIR}/Release CACHE PATH "Directory for Release builds" FORCE)
endif(CMAKE_BUILD_TYPE MATCHES Release)

if(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
   set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_RELWITHDEBINFO})
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CME_BINARY_DIR}/RelWithDebInfo CACHE PATH "Directory for RelWithDebInfo builds" FORCE)
endif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)

if(CMAKE_BUILD_TYPE MATCHES MinSizeRel)
   set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS_MINSIZEREL})
   set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CME_BINARY_DIR}/MinSizeRel CACHE PATH "Directory for MinSizeRel builds" FORCE)
endif(CMAKE_BUILD_TYPE MATCHES MinSizeRel)

#
# Continue setting up the compiler flags, next do those that are always set
#
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DCPU -Dx86")

#
# Now add different compiler flags for different platforms
# TODO this isn't finished
#
if(CMAKE_COMPILER_IS_GNUCC)				# gcc
   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -W -Wall -Wpointer-arith -Wcast-qual -Wcast-align -Wwrite-strings -Wredundant-decls")

   # For Debug builds under gcc
#   if(CMAKE_BUILD_TYPE MATCHES Debug)
#      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Weffc++")
#   endif(CMAKE_BUILD_TYPE MATCHES Debug)

   # For RelWithDebInfo builds under gcc, set flag for profiling with gprof
   if(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pg")
   endif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)

   # For Release builds under gcc, add an extra flag
   if(CMAKE_BUILD_TYPE MATCHES Release)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fomit-frame-pointer")
   endif(CMAKE_BUILD_TYPE MATCHES Release)

   # Finally set some gcc-specific linker flags
   set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--as-needed -Wl,--no-undefined")
endif(CMAKE_COMPILER_IS_GNUCC)

if(MSVC)
  if(MSVC60)						# MS Visual Studio 6.0
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
  else()						# more recent MS Visual Studio
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Wall /Wp64")
  endif(MSVC60)
endif(MSVC)

#
# The model cannot be built without GDAL
#
message("Finding libraries")
find_package(GDAL REQUIRED)
#message(STATUS "GDAL_LIBRARIES=${GDAL_LIBRARIES}")
#message(STATUS "GDAL_INCLUDE_DIRS=${GDAL_INCLUDE_DIRS}")
set(LIBS ${LIBS} ${GDAL_LIBRARIES})
set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} ${GDAL_INCLUDE_DIRS})
#message(STATUS "LIBS=${LIBS}")
#message(STATUS "CMAKE_INCLUDE_PATH=${CMAKE_INCLUDE_PATH}")

#
# However, OpenMP is optional and should never be linked in a Debug build
#
#if(NOT CMAKE_BUILD_TYPE MATCHES Debug)
#   find_package(OpenMP)
#   if(OPENMP_FOUND)
#      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
#   endif(OPENMP_FOUND)
#endif(NOT CMAKE_BUILD_TYPE MATCHES Debug)

#
# Finally, the important bits
#
include_directories(${CME_SOURCE_DIR} ${CMAKE_INCLUDE_PATH})
add_executable(${CME_EXECUTABLE} ${CME_SOURCE_FILES})
target_link_libraries(${CME_EXECUTABLE} ${LIBS})
install(TARGETS ${CME_EXECUTABLE} RUNTIME DESTINATION ${CME_INSTALL_DIR})

#
# Tell the user what has happened
#
message("Serial Makefile written")
message(STATUS "CMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}")
message(STATUS "CMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS}")
#message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "CMAKE_INCLUDE_PATH=${CMAKE_INCLUDE_PATH}")
message(STATUS "LIBS=${LIBS}")
message(STATUS "CME_INSTALL_DIR=${CME_INSTALL_DIR}")
message(STATUS "CMAKE_PREFIX_PATH=${CMAKE_PREFIX_PATH}")
message(STATUS "CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")


file(GLOB sources *.cpp roaring.c)
file(GLOB headers *.h *.hpp *.hh *.tcc)

list(FILTER sources EXCLUDE REGEX "[Bb][Ii][Ff][Rr][Oo][Ss][Tt]\\.cpp")

add_definitions(-DMAX_KMER_SIZE=${MAX_KMER_SIZE})
add_definitions(-DMAX_GMER_SIZE=${MAX_GMER_SIZE})

add_library(bifrost_static STATIC ${sources} ${headers})
add_library(bifrost_dynamic SHARED ${sources} ${headers})

set_target_properties(bifrost_static PROPERTIES OUTPUT_NAME "bifrost")
set_target_properties(bifrost_dynamic PROPERTIES OUTPUT_NAME "bifrost")

target_include_directories(bifrost_static PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(bifrost_dynamic PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT MSVC)
  add_executable(Bifrost Bifrost.cpp)
endif()

find_package(Threads REQUIRED)
if(UNIX)
  target_link_libraries(bifrost_static pthread)
  target_link_libraries(bifrost_dynamic pthread)
elseif(WIN32)
  target_link_libraries(bifrost_static psapi)
  target_link_libraries(bifrost_dynamic psapi)
endif()

find_package(ZLIB REQUIRED)
target_link_libraries(bifrost_static ${ZLIB_LIBRARIES})
target_link_libraries(bifrost_dynamic ${ZLIB_LIBRARIES})

if (ZLIB_FOUND)
    if (DEFINED ZLIB_INCLUDE_DIRS)
        include_directories( ${ZLIB_INCLUDE_DIRS} )
    elseif (DEFINED ZLIB_INCLUDE_DIR)
        include_directories( ${ZLIB_INCLUDE_DIR} )
    else()
        message(FATAL_ERROR "zlib found but no include directories are set.")
    endif()
else()
    message(FATAL_ERROR "zlib not found. Required for to output files")
endif(ZLIB_FOUND)

if(NOT MSVC)
  target_link_libraries(Bifrost bifrost_dynamic)
endif()

if(NOT MSVC)
  install(TARGETS Bifrost DESTINATION bin)
endif()
install(TARGETS bifrost_dynamic DESTINATION lib)
install(TARGETS bifrost_static DESTINATION lib)
install(FILES ${headers} DESTINATION include/bifrost)

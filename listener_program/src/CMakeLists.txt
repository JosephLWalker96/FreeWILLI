# Check if static linking is enabled
if (CMAKE_EXE_LINKER_FLAGS MATCHES "-static")
    set(STATIC_BUILD TRUE)
    message(STATUS "Building with static linking: src/ML will be excluded")
else()
    set(STATIC_BUILD FALSE)
endif()

# Recursively collect all source files in src/ and subdirectories
file(GLOB_RECURSE MAINLIB_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp         
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp
)

# If STATIC_BUILD is enabled, remove src/ML from the list
if (STATIC_BUILD)
    list(FILTER MAINLIB_SOURCES EXCLUDE REGEX "${CMAKE_CURRENT_SOURCE_DIR}/ML/.*\\.cpp")
endif()

# Create the MainLib target
add_library(MainLib STATIC ${MAINLIB_SOURCES})

# Include directories, excluding src/ML for static builds
target_include_directories(MainLib PRIVATE
    ${THIRD_PARTY_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/libs/dbscan
    ${PROJECT_SOURCE_DIR}/src/algorithms
    ${PROJECT_SOURCE_DIR}/src/io
    ${PROJECT_SOURCE_DIR}/src/threads
    ${PROJECT_SOURCE_DIR}/src/tracker
)

# Conditionally include ML only for non-static builds
if (NOT STATIC_BUILD)
    target_include_directories(MainLib PRIVATE ${PROJECT_SOURCE_DIR}/src/ML)
endif()

# Link libraries to MainLib
target_link_libraries(MainLib PRIVATE
    ${THIRD_PARTY_LIBRARIES}
    dbscan                                # Link the dbscan library
)

# Add executable target
add_executable(Listener main.cpp)

# Precompiled headers
target_precompile_headers(MainLib PRIVATE pch.h)
get_target_property(PCH_SOURCES Listener PRECOMPILE_HEADERS)
message(STATUS "Precompiled headers: ${PCH_SOURCES}")

# Include directories and link libraries for Listener
target_include_directories(Listener PRIVATE ${THIRD_PARTY_INCLUDE_DIRS})
target_link_libraries(Listener PRIVATE MainLib ${THIRD_PARTY_LIBRARIES})

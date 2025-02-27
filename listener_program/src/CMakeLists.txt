
# Recursively collect all source files in src/ and subdirectories
file(GLOB_RECURSE MAINLIB_SOURCES 
    ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp         
    ${CMAKE_CURRENT_SOURCE_DIR}/*/*.cpp
)

# Create the MainLib target
add_library(MainLib STATIC ${MAINLIB_SOURCES})

target_include_directories(MainLib PRIVATE
    ${THIRD_PARTY_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/libs/dbscan
    ${PROJECT_SOURCE_DIR}/src/algorithms
    ${PROJECT_SOURCE_DIR}/src/io
    ${PROJECT_SOURCE_DIR}/src/ML
    ${PROJECT_SOURCE_DIR}/src/threads
    ${PROJECT_SOURCE_DIR}/src/tracker
)

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

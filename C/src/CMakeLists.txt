# Add MainLib target
#add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp listener_thread.cpp processor_thread.cpp kalman_filter.cpp, tracker.cpp)

# Add ONNX Runtime include directories to MainLib
#target_include_directories(MainLib PRIVATE ${THIRD_PARTY_INCLUDE_DIRS})

# Link ONNX Runtime libraries to MainLib
#target_link_libraries(MainLib PRIVATE ${THIRD_PARTY_LIBRARIES})

# Add executable target
#add_executable(HarpListen main.cpp)

# Precompiled headers
#target_precompile_headers(MainLib PRIVATE pch.h)
#get_target_property(PCH_SOURCES HarpListen PRECOMPILE_HEADERS)
#message(STATUS "Precompiled headers: ${PCH_SOURCES}")

# Include directories and link libraries for HarpListen
#target_include_directories(HarpListen PRIVATE ${THIRD_PARTY_INCLUDE_DIRS})
#target_link_libraries(HarpListen PRIVATE MainLib ${THIRD_PARTY_LIBRARIES})

# src/CMakeLists.txt

# Add MainLib target
add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp listener_thread.cpp processor_thread.cpp socket_manager.cpp kalman_filter.cpp tracker.cpp onnx_model.cpp)

# Include directories for MainLib
target_include_directories(MainLib PRIVATE
    ${THIRD_PARTY_INCLUDE_DIRS}
    ${PROJECT_SOURCE_DIR}/libs/dbscan   # Add dbscan include directory
)

# Link libraries to MainLib
target_link_libraries(MainLib PRIVATE
    ${THIRD_PARTY_LIBRARIES}
    dbscan                                # Link the dbscan library
)

# Add executable target
add_executable(HarpListen main.cpp)

# Precompiled headers
target_precompile_headers(MainLib PRIVATE pch.h)
get_target_property(PCH_SOURCES HarpListen PRECOMPILE_HEADERS)
message(STATUS "Precompiled headers: ${PCH_SOURCES}")

# Include directories and link libraries for HarpListen
target_include_directories(HarpListen PRIVATE ${THIRD_PARTY_INCLUDE_DIRS})
target_link_libraries(HarpListen PRIVATE MainLib ${THIRD_PARTY_LIBRARIES})
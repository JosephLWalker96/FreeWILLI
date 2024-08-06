add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp)

add_executable(HarpListen main.cpp)

# Precompiled headers
target_precompile_headers(MainLib PRIVATE pch.h)
get_target_property(PCH_SOURCES HarpListen PRECOMPILE_HEADERS)
message(STATUS "Precompiled headers: ${PCH_SOURCES}")

include_directories(HarpListen PRIVATE
        ${THIRD_PARTY_INCLUDE_DIRS}
)
target_link_libraries(HarpListen PRIVATE
        MainLib
        ${THIRD_PARTY_LIBRARIES}
)
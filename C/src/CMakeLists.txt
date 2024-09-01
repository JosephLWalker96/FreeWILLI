add_executable(HarpListen main.cpp)

# Precompiled headers
if (${CMAKE_BUILD_TYPE} STREQUAL "PicoW")
    add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp pch.h)
    include_directories(MainLib PRIVATE
            ${THIRD_PARTY_INCLUDE_DIRS}
    )
    target_link_libraries(MainLib PUBLIC
            ${THIRD_PARTY_LIBRARIES}
            fftw3f
    )
    set(PICO "PICO")
    target_compile_definitions(MainLib PUBLIC PICO)

    get_target_property(PCH_SOURCES HarpListen PRECOMPILE_HEADERS)
    include_directories(HarpListen PRIVATE
            ${THIRD_PARTY_INCLUDE_DIRS}
    )
    target_link_libraries(HarpListen PRIVATE
            MainLib
#            ${THIRD_PARTY_LIBRARIES}
            LWIP_PORT
    )

    set(PICO "PICO")
    pico_enable_stdio_usb(HarpListen 1)
    pico_enable_stdio_uart(HarpListen 0)
    pico_add_extra_outputs(HarpListen)
else ()
    add_library(MainLib STATIC process_data.cpp TDOA_estimation.cpp utils.cpp)
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
endif ()
macro(add_boolean_option name val helpstr adddef)
  if(DEFINED ${name})
    set(value ${${name}})
  else(DEFINED ${name})
    set(value ${val})
  endif()
  set(${name} ${value} CACHE STRING "${helpstr}")
  set_property(CACHE ${name} PROPERTY TYPE BOOL)
  if (${value} AND ${adddef})
    add_definitions("-D${name}")
  endif (${value} AND ${adddef})
endmacro(add_boolean_option)

find_package(cppzmq CONFIG REQUIRED)

include(FetchContent)

FetchContent_Declare(
    flatbuffers
    GIT_REPOSITORY https://github.com/google/flatbuffers.git
    GIT_TAG v25.2.10
)

FetchContent_MakeAvailable(flatbuffers)


set(SCHEMA_FILE "${CMAKE_CURRENT_SOURCE_DIR}/srs_schema.fbs")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}")
set(GENERATED_HEADER "${GENERATED_DIR}/srs_schema_generated.h")

add_custom_command(
    OUTPUT "${GENERATED_HEADER}"

    COMMAND "${CMAKE_COMMAND}" -E make_directory "${GENERATED_DIR}"

    COMMAND $<TARGET_FILE:flatc>
            --cpp
            -o "${GENERATED_DIR}"
            "${SCHEMA_FILE}"

    DEPENDS
        flatc
        "${SCHEMA_FILE}"

    VERBATIM
)

add_boolean_option(OAI_AERIAL OFF "Activate OAI's AERIAL driver" OFF)
add_subdirectory(localization_plots)

add_executable(xapp_srs_ind
		xapp_srs_indication.c
                ../../../../src/util/alg_ds/alg/defer.c
                srs_fapi/debug.c
                srs_fapi/nfapi.c
                srs_fapi/srs_fapi_p7.c
              )

if (OAI_AERIAL)
target_compile_definitions(xapp_srs_ind PRIVATE ENABLE_AERIAL)
target_compile_definitions(xapp_srs_ind PRIVATE FAPI_BYTE_ORDERING_BIG_ENDIAN)
endif()
 target_link_libraries(xapp_srs_ind
                      PUBLIC
                      e42_xapp
                      -pthread
                      -lsctp
                      -ldl
                      )


add_executable(xapp_srs_channel
                xapp_srs_channel.cpp
                srs_fapi/debug.c
                srs_fapi/nfapi.c
                srs_fapi/srs_fapi_p7.c
                "${GENERATED_HEADER}"
              )

target_include_directories(xapp_srs_channel PRIVATE  "${GENERATED_DIR}")
if (OAI_AERIAL)
target_compile_definitions(xapp_srs_channel PRIVATE ENABLE_AERIAL)
target_compile_definitions(xapp_srs_channel PRIVATE FAPI_BYTE_ORDERING_BIG_ENDIAN)
endif()
 target_link_libraries(xapp_srs_channel
                      PRIVATE
                         flatbuffers
			                   cppzmq
                      PUBLIC
                        e42_xapp
                        -pthread
                        -lsctp
                        -ldl
                        m
                     )

add_executable(zmq_cl zmq_client.cpp "${GENERATED_HEADER}")
target_include_directories(zmq_cl PRIVATE  "${GENERATED_DIR}")
target_link_libraries(zmq_cl PRIVATE cppzmq flatbuffers)


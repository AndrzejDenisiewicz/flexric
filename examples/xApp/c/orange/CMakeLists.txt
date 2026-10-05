# SPDX-License-Identifier: LicenseRef-CSSL-1.0

include_directories(src/xApp)

add_executable(xapp_es_with_cell_util
    xapp_es_with_cell_util.c
    ../../../../src/util/alg_ds/alg/defer.c
)

add_executable(rf_reconfiguration_xapp
    rf_reconfiguration_xapp.c
    rf_reconfiguration_xapp_no_rrc.c
    ../../../../src/util/alg_ds/alg/defer.c
    ../../../../src/util/alg_ds/alg/murmur_hash_32.c
    ../../../../src/util/alg_ds/ds/assoc_container/assoc_ht_open_address.c
)

target_include_directories(rf_reconfiguration_xapp
    PRIVATE
    ../../../../src/sm/kpm_sm/kpm_sm_v03.00/ie/asn
    ../../../../src/lib/e2ap/v3_01/ie/asn
)

target_compile_definitions(rf_reconfiguration_xapp
    PRIVATE
    KPM_MEAS_LIST="${CMAKE_BINARY_DIR}/28_552_kpm_meas.txt"
)

# Link necessary libraries
target_link_libraries(xapp_es_with_cell_util
    PUBLIC
    e42_xapp
    pthread
    sctp
    dl
)

target_link_libraries(rf_reconfiguration_xapp
    PUBLIC
    e42_xapp
    pthread
    sctp
    dl
)

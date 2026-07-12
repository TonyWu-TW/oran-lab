# SPDX-License-Identifier: LicenseRef-CSSL-1.0

include_directories(src/xApp)

add_executable(xapp_es_with_cell_util
    xapp_es_with_cell_util.c
    ../../../../src/util/alg_ds/alg/defer.c
)

# Link necessary libraries
target_link_libraries(xapp_es_with_cell_util
    PUBLIC
    e42_xapp
    pthread
    sctp
    dl
)

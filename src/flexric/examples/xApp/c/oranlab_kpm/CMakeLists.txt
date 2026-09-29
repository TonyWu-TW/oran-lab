# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_executable(oranlab_kpm oranlab_kpm.c)
target_link_libraries(oranlab_kpm PUBLIC e42_xapp -pthread -lsctp -ldl)

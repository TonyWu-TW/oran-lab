# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_executable(e2_hello
  e2_hello.c
  ../../../../src/util/alg_ds/alg/defer.c)
target_link_libraries(e2_hello PUBLIC e42_xapp -pthread -lsctp -ldl)

add_executable(e2_kpm_sub
  e2_kpm_sub.c
  ../../../../src/util/alg_ds/alg/defer.c)
target_link_libraries(e2_kpm_sub PUBLIC e42_xapp -pthread -lsctp -ldl)

add_executable(e2_rc_ctrl e2_rc_ctrl.c)
target_link_libraries(e2_rc_ctrl PUBLIC e42_xapp -pthread -lsctp -ldl)

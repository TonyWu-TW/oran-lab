# SPDX-License-Identifier: LicenseRef-CSSL-1.0

add_executable(voiceguard_rc voiceguard_rc.c)

target_link_libraries(voiceguard_rc
                      PUBLIC
                      e42_xapp
                      -pthread
                      -lsctp
                      -ldl)

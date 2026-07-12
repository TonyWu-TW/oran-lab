# SPDX-License-Identifier: LicenseRef-CSSL-1.0


add_library(e2ap_ep_obj OBJECT e2ap_ep.c sctp_msg.c )
target_link_libraries(e2ap_ep_obj PRIVATE -lsctp)




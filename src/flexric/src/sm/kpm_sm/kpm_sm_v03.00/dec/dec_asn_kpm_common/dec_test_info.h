/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef DECODING_TEST_INFORMATION_H
#define DECODING_TEST_INFORMATION_H

#include "../../ie/asn/TestCondInfo.h"
#include "../../ie/kpm_data_ie/data/test_info_lst.h"

test_info_lst_t kpm_dec_test_info_asn(const TestCondInfo_t * test_info);


#endif

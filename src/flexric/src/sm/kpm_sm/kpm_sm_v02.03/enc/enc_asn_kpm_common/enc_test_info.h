/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef ENCODING_TEST_INFORMATION_H
#define ENCODING_TEST_INFORMATION_H

#include "../../ie/asn/TestCondInfo.h"
#include "../../ie/kpm_data_ie/data/test_info_lst.h"

TestCondInfo_t kpm_enc_test_info_asn(const test_info_lst_t * test_info);


#endif

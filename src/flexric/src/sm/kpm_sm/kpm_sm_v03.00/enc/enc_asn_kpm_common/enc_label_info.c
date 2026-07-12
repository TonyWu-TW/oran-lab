/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdint.h>
#include <stdlib.h>
#include "enc_label_info.h"
#include "../../../../../util/conversions.h"

LabelInfoItem_t * kpm_enc_label_info_asn(const label_info_lst_t * label_info)
{
    LabelInfoItem_t * label_info_asn = calloc(1, sizeof(LabelInfoItem_t));
    assert (label_info_asn != NULL && "Memory exhausted");

    if (label_info->noLabel != NULL) {
      assert(*label_info->noLabel == TRUE_ENUM_VALUE && "has only one value (true)");
      label_info_asn->measLabel.noLabel = malloc (sizeof(*(label_info_asn->measLabel.noLabel)));
      assert (label_info_asn->measLabel.noLabel != NULL && "Memory exhausted");
      *label_info_asn->measLabel.noLabel = 0;
      /* 
       * specification mentions that if 'noLabel' is included, other elements in the same datastructure 
       * 'LabelInfoItem_t' shall not be included.
       */
      return label_info_asn;
    }      

    if (label_info->plmn_id != NULL){
      label_info_asn->measLabel.plmnID = calloc(1, sizeof(*label_info_asn->measLabel.plmnID));
      MCC_MNC_TO_PLMNID(label_info->plmn_id->mcc, label_info->plmn_id->mnc, label_info->plmn_id->mnc_digit_len, label_info_asn->measLabel.plmnID);
    }

    if (label_info->sliceID != NULL) {
      label_info_asn->measLabel.sliceID = calloc(1, sizeof(*label_info_asn->measLabel.sliceID));
      label_info_asn->measLabel.sliceID->sST.size = 1;
      label_info_asn->measLabel.sliceID->sST.buf = calloc(label_info_asn->measLabel.sliceID->sST.size, sizeof(uint8_t));
      *label_info_asn->measLabel.sliceID->sST.buf = label_info->sliceID->sST;

      if(label_info->sliceID->sD != NULL){
        label_info_asn->measLabel.sliceID->sD = calloc(1, sizeof(*label_info_asn->measLabel.sliceID));
        label_info_asn->measLabel.sliceID->sD->size = 3;
        label_info_asn->measLabel.sliceID->sD->buf = calloc(label_info_asn->measLabel.sliceID->sD->size, sizeof(uint8_t));
        INT24_TO_BUFFER(*label_info->sliceID->sD, label_info_asn->measLabel.sliceID->sD->buf);
      }
    }
    if (label_info->fiveQI != NULL) {
      label_info_asn->measLabel.fiveQI = calloc(1, sizeof(*label_info_asn->measLabel.fiveQI));
      *label_info_asn->measLabel.fiveQI = *label_info->fiveQI;
    }
    if (label_info->qFI != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->qCI != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->qCImax != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->qCImin != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->aRPmax != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->aRPmin != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->bitrateRange != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->layerMU_MIMO != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->sUM != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->distBinX != NULL) {
      label_info_asn->measLabel.distBinX = calloc(1, sizeof(*label_info_asn->measLabel.distBinX));
      assert(label_info_asn->measLabel.distBinX != NULL && "Memory exhausted");
      *label_info_asn->measLabel.distBinX = *label_info->distBinX;
    }
    if (label_info->distBinY != NULL) {
      label_info_asn->measLabel.distBinY = calloc(1, sizeof(*label_info_asn->measLabel.distBinY));
      assert(label_info_asn->measLabel.distBinY != NULL && "Memory exhausted");
      *label_info_asn->measLabel.distBinY = *label_info->distBinY;
    }
    if (label_info->distBinZ != NULL) {
      label_info_asn->measLabel.distBinZ = calloc(1, sizeof(*label_info_asn->measLabel.distBinZ));
      assert(label_info_asn->measLabel.distBinZ != NULL && "Memory exhausted");
      *label_info_asn->measLabel.distBinZ = *label_info->distBinZ;
    }
    if (label_info->preLabelOverride != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->startEndInd != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->min != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->max != NULL) {
      assert(false && "not implemented");
    }
    if (label_info->avg != NULL) {
      assert(*label_info->avg == TRUE_ENUM_VALUE && "has only one value (true)");
      label_info_asn->measLabel.avg = malloc (sizeof(*(label_info_asn->measLabel.avg)));
      assert (label_info_asn->measLabel.avg!= NULL && "Memory exhausted");
      *label_info_asn->measLabel.avg = 0; // or TRUE type
    }

    return label_info_asn;


}

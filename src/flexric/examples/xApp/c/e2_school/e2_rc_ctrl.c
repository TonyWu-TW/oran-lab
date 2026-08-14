/*
 * e2_rc_ctrl — FlexRIC xApp API 第 2 步：送一筆 E2SM-RC Control
 *
 * E2 上「下指令」不是改 Manager / Prometheus，而是：
 *   1. 找到 RAN Function 3（RC）
 *   2. 組 rc_ctrl_req_data_t（style + action + UE id + RAN parameters）
 *   3. control_sm_xapp_api() → RIC 轉成 E2 Control Request
 *   4. 看 gNB 有沒有 ACK
 *
 * 這支程式送 Style 2 / Action 6（RRM Policy Ratio）：
 *   min=0 max=100 dedicated=0
 * 對這套 OCUDU 那是「單次 grant size 上限」，不是總頻寬配額。
 * 預設只打 ue_id=0，可用環境變數改。
 */

#include "../../../../src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "../../../../src/sm/rc_sm/ie/ir/ran_param_struct.h"
#include "../../../../src/xApp/e42_xapp_api.h"

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
  RC_RAN_FUNCTION = 3,
  RRM_POLICY_RATIO_LIST = 1,
  RRM_POLICY_RATIO_GROUP = 2,
  RRM_POLICY_MEMBER = 6,
  PLMN_IDENTITY = 7,
  SST = 9,
  SD = 10,
  MIN_PRB_POLICY_RATIO = 11,
  MAX_PRB_POLICY_RATIO = 12,
  DEDICATED_PRB_POLICY_RATIO = 13,
};

static long env_long(const char* name, long fallback)
{
  const char* value = getenv(name);
  if (value == NULL || value[0] == '\0') {
    return fallback;
  }
  errno = 0;
  char* end = NULL;
  long parsed = strtol(value, &end, 10);
  if (errno != 0 || end == value || *end != '\0') {
    fprintf(stderr, "invalid %s=%s\n", name, value);
    exit(EXIT_FAILURE);
  }
  return parsed;
}

static byte_array_t octets(const uint8_t* bytes, size_t length)
{
  byte_array_t result = {.len = length, .buf = calloc(length, 1)};
  memcpy(result.buf, bytes, length);
  return result;
}

static seq_ran_param_t integer_parameter(uint32_t id, int64_t value)
{
  seq_ran_param_t parameter = {0};
  parameter.ran_param_id = id;
  parameter.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  parameter.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  parameter.ran_param_val.flag_false->type = INTEGER_RAN_PARAMETER_VALUE;
  parameter.ran_param_val.flag_false->int_ran = value;
  return parameter;
}

static seq_ran_param_t octet_parameter(uint32_t id, const uint8_t* bytes, size_t length)
{
  seq_ran_param_t parameter = {0};
  parameter.ran_param_id = id;
  parameter.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  parameter.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  parameter.ran_param_val.flag_false->type = OCTET_STRING_RAN_PARAMETER_VALUE;
  parameter.ran_param_val.flag_false->octet_str_ran = octets(bytes, length);
  return parameter;
}

static seq_ran_param_t structure_parameter(uint32_t id, size_t child_count)
{
  seq_ran_param_t parameter = {0};
  parameter.ran_param_id = id;
  parameter.ran_param_val.type = STRUCTURE_RAN_PARAMETER_VAL_TYPE;
  parameter.ran_param_val.strct = calloc(1, sizeof(ran_param_struct_t));
  parameter.ran_param_val.strct->sz_ran_param_struct = child_count;
  parameter.ran_param_val.strct->ran_param_struct = calloc(child_count, sizeof(seq_ran_param_t));
  return parameter;
}

static rc_ctrl_req_data_t make_control(uint32_t ue_id, uint8_t sst, uint32_t sd,
                                       int64_t minimum, int64_t maximum, int64_t dedicated)
{
  rc_ctrl_req_data_t control = {0};
  control.hdr.format = FORMAT_1_E2SM_RC_CTRL_HDR;
  control.hdr.frmt_1.ric_style_type = 2;
  control.hdr.frmt_1.ctrl_act_id = 6;
  control.hdr.frmt_1.ue_id.type = GNB_DU_UE_ID_E2SM;
  control.hdr.frmt_1.ue_id.gnb_du.gnb_cu_ue_f1ap = ue_id;

  control.msg.format = FORMAT_1_E2SM_RC_CTRL_MSG;
  control.msg.frmt_1.sz_ran_param = 1;
  control.msg.frmt_1.ran_param = calloc(1, sizeof(seq_ran_param_t));

  seq_ran_param_t ratio_list = {0};
  ratio_list.ran_param_id = RRM_POLICY_RATIO_LIST;
  ratio_list.ran_param_val.type = LIST_RAN_PARAMETER_VAL_TYPE;
  ratio_list.ran_param_val.lst = calloc(1, sizeof(ran_param_list_t));
  ratio_list.ran_param_val.lst->sz_lst_ran_param = 1;
  ratio_list.ran_param_val.lst->lst_ran_param = calloc(1, sizeof(lst_ran_param_t));

  ran_param_struct_t* list_item = &ratio_list.ran_param_val.lst->lst_ran_param[0].ran_param_struct;
  list_item->sz_ran_param_struct = 1;
  list_item->ran_param_struct = calloc(1, sizeof(seq_ran_param_t));
  list_item->ran_param_struct[0] = structure_parameter(RRM_POLICY_RATIO_GROUP, 4);

  seq_ran_param_t* group = list_item->ran_param_struct[0].ran_param_val.strct->ran_param_struct;
  group[0] = structure_parameter(RRM_POLICY_MEMBER, 3);

  /* PLMN 999-70，跟這套 lab 的 Open5GS / gNB 一致。 */
  const uint8_t plmn[3] = {0x99, 0xf9, 0x07};
  const uint8_t sst_bytes[1] = {sst};
  const uint8_t sd_bytes[3] = {
      (uint8_t)((sd >> 16) & 0xff),
      (uint8_t)((sd >> 8) & 0xff),
      (uint8_t)(sd & 0xff),
  };
  seq_ran_param_t* member = group[0].ran_param_val.strct->ran_param_struct;
  member[0] = octet_parameter(PLMN_IDENTITY, plmn, sizeof(plmn));
  member[1] = octet_parameter(SST, sst_bytes, sizeof(sst_bytes));
  member[2] = octet_parameter(SD, sd_bytes, sizeof(sd_bytes));
  group[1] = integer_parameter(MIN_PRB_POLICY_RATIO, minimum);
  group[2] = integer_parameter(MAX_PRB_POLICY_RATIO, maximum);
  group[3] = integer_parameter(DEDICATED_PRB_POLICY_RATIO, dedicated);

  control.msg.frmt_1.ran_param[0] = ratio_list;
  return control;
}

int main(int argc, char* argv[])
{
  const uint32_t ue_id = (uint32_t)env_long("E2_SCHOOL_UE_ID", 0);
  const int64_t minimum = env_long("E2_SCHOOL_MIN_PRB", 0);
  const int64_t maximum = env_long("E2_SCHOOL_MAX_PRB", 100);
  const int64_t dedicated = env_long("E2_SCHOOL_DEDICATED_PRB", 0);
  const uint8_t sst = (uint8_t)env_long("E2_SCHOOL_SST", 1);
  const uint32_t sd = (uint32_t)env_long("E2_SCHOOL_SD", 0xffffff);

  fr_args_t args = init_fr_args(argc, argv);
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  bool sent = false;
  bool ok = false;

  for (size_t i = 0; i < nodes.len; ++i) {
    e2_node_connected_xapp_t* node = &nodes.n[i];
    size_t index = node->len_rf;
    for (size_t r = 0; r < node->len_rf; ++r) {
      if (node->rf[r].id == RC_RAN_FUNCTION) {
        index = r;
        break;
      }
    }
    if (index >= node->len_rf || node->rf[index].defn.type != RC_RAN_FUNC_DEF_E) {
      continue;
    }

    rc_ctrl_req_data_t control = make_control(ue_id, sst, sd, minimum, maximum, dedicated);
    sm_ans_xapp_t answer = control_sm_xapp_api(&node->id, RC_RAN_FUNCTION, &control);
    sent = true;
    ok = answer.success;
    printf("RC control  success=%s  ue_id=%" PRIu32 "  min=%" PRId64
           "  max=%" PRId64 "  dedicated=%" PRId64 "  style=2 action=6\n",
           answer.success ? "true" : "false",
           ue_id, minimum, maximum, dedicated);
    if (!answer.success && answer.u.reason != NULL) {
      fprintf(stderr, "RC error: %s\n", answer.u.reason);
    }
    free_rc_ctrl_req_data(&control);
    break;
  }

  free_e2_node_arr_xapp(&nodes);
  while (!try_stop_xapp_api()) {
    usleep(1000);
  }
  if (!sent) {
    fprintf(stderr, "No E2 node advertising RC RAN function 3\n");
  }
  return sent && ok ? 0 : 1;
}

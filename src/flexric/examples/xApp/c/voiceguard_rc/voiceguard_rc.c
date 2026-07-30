/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 *
 * One-shot E2SM-RC Style 2 / Action 6 control bridge for VoiceGuard.
 *
 * FlexRIC's Python SDK does not expose the standardized RC service model in
 * this build.  The long-running policy remains in Python and invokes this
 * small native program for each idempotent PRB policy update.
 */

#include "../../../../src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "../../../../src/sm/rc_sm/ie/ir/ran_param_struct.h"
#include "../../../../src/xApp/e42_xapp_api.h"

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum {
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

typedef struct {
  uint32_t ue_id;
  int64_t minimum;
  int64_t maximum;
  int64_t dedicated;
} ue_policy_t;

static long env_long(const char* name, long default_value, long minimum, long maximum)
{
  const char* value = getenv(name);
  if (value == NULL || value[0] == '\0') {
    return default_value;
  }
  errno = 0;
  char* end = NULL;
  long parsed = strtol(value, &end, 10);
  if (errno != 0 || end == value || *end != '\0' || parsed < minimum || parsed > maximum) {
    fprintf(stderr, "VOICEGUARD_RC_ERROR invalid %s=%s (expected %ld..%ld)\n",
            name, value, minimum, maximum);
    exit(EXIT_FAILURE);
  }
  return parsed;
}

static byte_array_t octets(const uint8_t* bytes, size_t length)
{
  byte_array_t result = {0};
  result.len = length;
  result.buf = calloc(length, sizeof(uint8_t));
  if (result.buf == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  memcpy(result.buf, bytes, length);
  return result;
}

static seq_ran_param_t integer_parameter(uint32_t id, int64_t value)
{
  seq_ran_param_t parameter = {0};
  parameter.ran_param_id = id;
  parameter.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  parameter.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  if (parameter.ran_param_val.flag_false == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
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
  if (parameter.ran_param_val.flag_false == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
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
  if (parameter.ran_param_val.strct == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  parameter.ran_param_val.strct->sz_ran_param_struct = child_count;
  parameter.ran_param_val.strct->ran_param_struct = calloc(child_count, sizeof(seq_ran_param_t));
  if (parameter.ran_param_val.strct->ran_param_struct == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  return parameter;
}

static rc_ctrl_req_data_t make_control(uint32_t ue_id,
                                       uint8_t sst,
                                       uint32_t sd,
                                       int64_t minimum,
                                       int64_t maximum,
                                       int64_t dedicated)
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
  if (control.msg.frmt_1.ran_param == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }

  seq_ran_param_t ratio_list = {0};
  ratio_list.ran_param_id = RRM_POLICY_RATIO_LIST;
  ratio_list.ran_param_val.type = LIST_RAN_PARAMETER_VAL_TYPE;
  ratio_list.ran_param_val.lst = calloc(1, sizeof(ran_param_list_t));
  if (ratio_list.ran_param_val.lst == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  ratio_list.ran_param_val.lst->sz_lst_ran_param = 1;
  ratio_list.ran_param_val.lst->lst_ran_param = calloc(1, sizeof(lst_ran_param_t));
  if (ratio_list.ran_param_val.lst->lst_ran_param == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }

  ran_param_struct_t* list_item =
      &ratio_list.ran_param_val.lst->lst_ran_param[0].ran_param_struct;
  list_item->sz_ran_param_struct = 1;
  list_item->ran_param_struct = calloc(1, sizeof(seq_ran_param_t));
  if (list_item->ran_param_struct == NULL) {
    perror("calloc");
    exit(EXIT_FAILURE);
  }
  list_item->ran_param_struct[0] = structure_parameter(RRM_POLICY_RATIO_GROUP, 4);

  seq_ran_param_t* group =
      list_item->ran_param_struct[0].ran_param_val.strct->ran_param_struct;
  group[0] = structure_parameter(RRM_POLICY_MEMBER, 3);

  /* PLMN 999-70 encoded as TS 38.413 PLMN Identity: 99 F9 07. */
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

static size_t find_rc_function(const e2_node_connected_xapp_t* node)
{
  for (size_t i = 0; i < node->len_rf; ++i) {
    if (node->rf[i].id == 3) {
      return i;
    }
  }
  return node->len_rf;
}

static size_t load_policies(ue_policy_t* policies, size_t capacity)
{
  const char* batch = getenv("VOICEGUARD_POLICIES");
  if (batch == NULL || batch[0] == '\0') {
    policies[0] = (ue_policy_t){
        .ue_id = (uint32_t)env_long("VOICEGUARD_UE_ID", 2, 0, UINT32_MAX),
        .minimum = env_long("VOICEGUARD_MIN_PRB", 0, 0, 100),
        .maximum = env_long("VOICEGUARD_MAX_PRB", 100, 0, 100),
        .dedicated = env_long("VOICEGUARD_DEDICATED_PRB", 0, 0, 100),
    };
    return 1;
  }

  char* copy = strdup(batch);
  if (copy == NULL) {
    perror("strdup");
    exit(EXIT_FAILURE);
  }
  size_t count = 0;
  char* save = NULL;
  for (char* item = strtok_r(copy, ",", &save);
       item != NULL;
       item = strtok_r(NULL, ",", &save)) {
    unsigned ue = 0;
    long long minimum = 0;
    long long maximum = 0;
    long long dedicated = 0;
    char extra = '\0';
    if (count >= capacity ||
        sscanf(item, "%u:%lld:%lld:%lld%c",
               &ue, &minimum, &maximum, &dedicated, &extra) != 4 ||
        minimum < 0 || minimum > 100 ||
        maximum < 0 || maximum > 100 ||
        dedicated < 0 || dedicated > 100 ||
        minimum > maximum || dedicated > minimum) {
      fprintf(stderr,
              "VOICEGUARD_RC_ERROR invalid VOICEGUARD_POLICIES item '%s'; "
              "expected ue:min:max:dedicated with dedicated <= min <= max\n",
              item);
      free(copy);
      exit(EXIT_FAILURE);
    }
    policies[count++] = (ue_policy_t){
        .ue_id = ue,
        .minimum = minimum,
        .maximum = maximum,
        .dedicated = dedicated,
    };
  }
  free(copy);
  if (count == 0) {
    fprintf(stderr, "VOICEGUARD_RC_ERROR VOICEGUARD_POLICIES is empty\n");
    exit(EXIT_FAILURE);
  }
  return count;
}

int main(int argc, char* argv[])
{
  ue_policy_t policies[16] = {0};
  const size_t policy_count = load_policies(policies, 16);
  const uint8_t sst = (uint8_t)env_long("VOICEGUARD_SST", 1, 0, UINT8_MAX);
  const uint32_t sd = (uint32_t)env_long("VOICEGUARD_SD", 0xffffff, 0, 0xffffff);
  for (size_t i = 0; i < policy_count; ++i) {
    if (policies[i].minimum > policies[i].maximum ||
        policies[i].dedicated > policies[i].minimum) {
      fprintf(stderr, "VOICEGUARD_RC_ERROR invalid policy ordering\n");
      return EXIT_FAILURE;
    }
  }

  fr_args_t args = init_fr_args(argc, argv);
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  bool sent = false;
  bool succeeded = true;
  for (size_t i = 0; i < nodes.len; ++i) {
    e2_node_connected_xapp_t* node = &nodes.n[i];
    size_t index = find_rc_function(node);
    if (index >= node->len_rf || node->rf[index].defn.type != RC_RAN_FUNC_DEF_E ||
        node->rf[index].defn.rc.ctrl == NULL) {
      continue;
    }

    for (size_t policy_index = 0; policy_index < policy_count; ++policy_index) {
      const ue_policy_t* policy = &policies[policy_index];
      rc_ctrl_req_data_t control = make_control(
          policy->ue_id, sst, sd, policy->minimum, policy->maximum, policy->dedicated);
      sm_ans_xapp_t answer = control_sm_xapp_api(&node->id, 3, &control);
      sent = true;
      succeeded = succeeded && answer.success;
      printf("VOICEGUARD_RC_RESULT success=%s ue_id=%" PRIu32
             " min=%" PRId64 " max=%" PRId64 " dedicated=%" PRId64
             " sst=%" PRIu8 " sd=%06" PRIx32 "\n",
             answer.success ? "true" : "false", policy->ue_id, policy->minimum,
             policy->maximum, policy->dedicated, sst, sd);
      if (!answer.success && answer.u.reason != NULL) {
        fprintf(stderr, "VOICEGUARD_RC_ERROR %s\n", answer.u.reason);
      }
      free_rc_ctrl_req_data(&control);
    }
    break;
  }

  free_e2_node_arr_xapp(&nodes);
  while (!try_stop_xapp_api()) {
    usleep(1000);
  }
  if (!sent) {
    fprintf(stderr, "VOICEGUARD_RC_ERROR no E2 node advertising RC RAN function 3\n");
  }
  return sent && succeeded ? EXIT_SUCCESS : EXIT_FAILURE;
}

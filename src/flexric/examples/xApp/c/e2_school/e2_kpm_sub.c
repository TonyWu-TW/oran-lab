/*
 * e2_kpm_sub — FlexRIC xApp API 第 1 步：訂閱 E2SM-KPM
 *
 * E2 上「讀資料」不是去拉 Prometheus，而是：
 *   1. 找到 RAN Function 2（KPM）
 *   2. 看 gNB 廣告了哪些 Report Style、哪些 measurement 名字
 *   3. 組一份 subscription（event trigger + action definition）
 *   4. report_sm_xapp_api() → RIC 轉成 E2 Subscription Request
 *   5. gNB 週期性回 E2 Indication，callback 被叫
 *   6. 結束時 rm_report_sm_xapp_api() 取消訂閱
 *
 * 這支程式預設跑 20 秒。沒有 gNB / 沒有 KPM 會直接印原因離開。
 */

#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/alg_ds/alg/defer.h"
#include "../../../../src/util/e.h"
#include "../../../../src/util/time_now_us.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum { KPM_RAN_FUNCTION = 2 };
static const uint64_t kPeriodMs = 1000;
static const int kRunSeconds = 20;

static size_t find_rf(const e2_node_connected_xapp_t* node, int id)
{
  for (size_t i = 0; i < node->len_rf; ++i) {
    if (node->rf[i].id == (uint16_t)id) {
      return i;
    }
  }
  return node->len_rf;
}

static void print_kpm_capabilities(const kpm_ran_function_def_t* kpm)
{
  printf("KPM event-trigger styles = %zu, report styles = %zu\n",
         kpm->sz_ric_event_trigger_style_list,
         kpm->sz_ric_report_style_list);

  for (size_t i = 0; i < kpm->sz_ric_report_style_list; ++i) {
    const ric_report_style_item_t* style = &kpm->ric_report_style_list[i];
    printf("  report style enum=%d  action_def_format=%d  meas=%zu\n",
           style->report_style_type,
           style->act_def_format_type,
           style->meas_info_for_action_lst_len);
    for (size_t j = 0; j < style->meas_info_for_action_lst_len; ++j) {
      char* name = cp_ba_to_str(style->meas_info_for_action_lst[j].name);
      printf("    - %s\n", name);
      free(name);
    }
  }
}

static void log_measurements(const kpm_ind_msg_format_1_t* msg)
{
  for (size_t d = 0; d < msg->meas_data_lst_len; ++d) {
    const meas_data_lst_t* data = &msg->meas_data_lst[d];
    size_t record = 0;
    for (size_t i = 0; i < msg->meas_info_lst_len; ++i) {
      const meas_info_format_1_lst_t* info = &msg->meas_info_lst[i];
      for (size_t z = 0; z < info->label_info_lst_len && record < data->meas_record_len; ++z) {
        const meas_record_lst_t* value = &data->meas_record_lst[record++];
        char* name = cp_ba_to_str(info->meas_type.name);
        if (value->value == INTEGER_MEAS_VALUE) {
          printf("    %s = %d\n", name, value->int_val);
        } else if (value->value == REAL_MEAS_VALUE) {
          printf("    %s = %.3f\n", name, value->real_val);
        } else {
          printf("    %s = (no value)\n", name);
        }
        free(name);
      }
    }
  }
}

static void sm_cb_kpm(sm_ag_if_rd_t const* rd)
{
  /* RIC 把 E2 Indication 解好後，丟進這個 callback。
   * rd->ind.kpm 就是 KPM 的 header + message。 */
  if (rd == NULL || rd->type != INDICATION_MSG_AGENT_IF_ANS_V0 ||
      rd->ind.type != KPM_STATS_V3_0) {
    return;
  }

  const kpm_ind_data_t* ind = &rd->ind.kpm.ind;
  const int64_t now = time_now_us();
  printf("\nKPM indication  latency=%ld us  msg_format=%d\n",
         now - ind->hdr.kpm_ric_ind_hdr_format_1.collectStartTime,
         ind->msg.type);

  if (ind->msg.type == FORMAT_1_INDICATION_MESSAGE) {
    log_measurements(&ind->msg.frm_1);
  } else if (ind->msg.type == FORMAT_3_INDICATION_MESSAGE) {
    for (size_t i = 0; i < ind->msg.frm_3.ue_meas_report_lst_len; ++i) {
      const meas_report_per_ue_t* ue = &ind->msg.frm_3.meas_report_per_ue[i];
      printf("  UE report %zu  ue_id_type=%d\n", i, ue->ue_meas_report_lst.type);
      log_measurements(&ue->ind_msg_format_1);
    }
  } else {
    printf("  unsupported indication format %d\n", ind->msg.type);
  }
}

static label_info_lst_t no_label(void)
{
  label_info_lst_t label = {0};
  label.noLabel = ecalloc(1, sizeof(enum_value_e));
  *label.noLabel = TRUE_ENUM_VALUE;
  return label;
}

static kpm_act_def_format_1_t fill_act_def_frm_1(const ric_report_style_item_t* style)
{
  kpm_act_def_format_1_t ad = {0};
  ad.meas_info_lst_len = style->meas_info_for_action_lst_len;
  ad.meas_info_lst = calloc(ad.meas_info_lst_len, sizeof(meas_info_format_1_lst_t));
  for (size_t i = 0; i < ad.meas_info_lst_len; ++i) {
    ad.meas_info_lst[i].meas_type.type = NAME_MEAS_TYPE;
    ad.meas_info_lst[i].meas_type.name = copy_byte_array(style->meas_info_for_action_lst[i].name);
    ad.meas_info_lst[i].label_info_lst_len = 1;
    ad.meas_info_lst[i].label_info_lst = ecalloc(1, sizeof(label_info_lst_t));
    ad.meas_info_lst[i].label_info_lst[0] = no_label();
  }
  ad.gran_period_ms = kPeriodMs;
  return ad;
}

static test_info_lst_t nssai_equals(int sst)
{
  /* Style 4 必須帶 matching condition。這份 lab 的 slice 是 sst=1。 */
  test_info_lst_t dst = {0};
  dst.test_cond_type = S_NSSAI_TEST_COND_TYPE;
  dst.S_NSSAI = TRUE_TEST_COND_TYPE;
  dst.test_cond = calloc(1, sizeof(test_cond_e));
  *dst.test_cond = EQUAL_TEST_COND;
  dst.test_cond_value = calloc(1, sizeof(test_cond_value_t));
  dst.test_cond_value->type = OCTET_STRING_TEST_COND_VALUE;
  dst.test_cond_value->octet_string_value = calloc(1, sizeof(byte_array_t));
  dst.test_cond_value->octet_string_value->len = 1;
  dst.test_cond_value->octet_string_value->buf = calloc(1, sizeof(uint8_t));
  dst.test_cond_value->octet_string_value->buf[0] = (uint8_t)sst;
  return dst;
}

static kpm_sub_data_t make_subscription(const kpm_ran_function_def_t* kpm,
                                        const ric_report_style_item_t* style)
{
  kpm_sub_data_t sub = {0};

  /* Event trigger = 多久報一次。KPM Format 1 就是 report_period_ms。 */
  sub.ev_trg_def.type = FORMAT_1_RIC_EVENT_TRIGGER;
  sub.ev_trg_def.kpm_ric_event_trigger_format_1.report_period_ms = kPeriodMs;

  sub.sz_ad = 1;
  sub.ad = calloc(1, sizeof(kpm_act_def_t));

  if (style->act_def_format_type == FORMAT_1_ACTION_DEFINITION) {
    /* Style 1：cell 級 KPI，沒有 per-UE matching。 */
    sub.ad->type = FORMAT_1_ACTION_DEFINITION;
    sub.ad->frm_1 = fill_act_def_frm_1(style);
  } else if (style->act_def_format_type == FORMAT_4_ACTION_DEFINITION) {
    /* Style 4：先用 S-NSSAI==1 過濾 UE，再套 Format 1 的量測清單。 */
    sub.ad->type = FORMAT_4_ACTION_DEFINITION;
    sub.ad->frm_4.matching_cond_lst_len = 1;
    sub.ad->frm_4.matching_cond_lst = calloc(1, sizeof(matching_condition_format_4_lst_t));
    sub.ad->frm_4.matching_cond_lst[0].test_info_lst = nssai_equals(1);
    sub.ad->frm_4.action_def_format_1 = fill_act_def_frm_1(style);
  } else {
    printf("skip unsupported action definition format %d\n", style->act_def_format_type);
  }
  return sub;
}

int main(int argc, char* argv[])
{
  fr_args_t args = init_fr_args(argc, argv);
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  defer({ free_e2_node_arr_xapp(&nodes); });
  if (nodes.len == 0) {
    printf("No E2 node. Start RIC + gNB first.\n");
    while (!try_stop_xapp_api()) usleep(1000);
    return 1;
  }

  sm_ans_xapp_t handles[16] = {0};
  size_t handle_count = 0;

  for (size_t n = 0; n < nodes.len; ++n) {
    e2_node_connected_xapp_t* node = &nodes.n[n];
    size_t idx = find_rf(node, KPM_RAN_FUNCTION);
    if (idx >= node->len_rf || node->rf[idx].defn.type != KPM_RAN_FUNC_DEF_E) {
      printf("E2 node %zu has no KPM RAN function 2\n", n);
      continue;
    }

    print_kpm_capabilities(&node->rf[idx].defn.kpm);

    const kpm_ran_function_def_t* kpm = &node->rf[idx].defn.kpm;
    for (size_t s = 0; s < kpm->sz_ric_report_style_list && handle_count < 16; ++s) {
      const ric_report_style_item_t* style = &kpm->ric_report_style_list[s];
      if (style->act_def_format_type != FORMAT_1_ACTION_DEFINITION &&
          style->act_def_format_type != FORMAT_4_ACTION_DEFINITION) {
        printf("skip report style enum=%d\n", style->report_style_type);
        continue;
      }

      kpm_sub_data_t sub = make_subscription(kpm, style);
      /* 這一行才是真正的 E2 Subscription Request。 */
      sm_ans_xapp_t ans = report_sm_xapp_api(&node->id, KPM_RAN_FUNCTION, &sub, sm_cb_kpm);
      free_kpm_sub_data(&sub);
      printf("subscribe style enum=%d  success=%s  handle=%d\n",
             style->report_style_type,
             ans.success ? "true" : "false",
             ans.success ? ans.u.handle : -1);
      if (ans.success) {
        handles[handle_count++] = ans;
      }
    }
  }

  if (handle_count == 0) {
    printf("No KPM subscription accepted.\n");
  } else {
    printf("Listening for %d seconds. Indications print below.\n", kRunSeconds);
    sleep(kRunSeconds);
  }

  for (size_t i = 0; i < handle_count; ++i) {
    rm_report_sm_xapp_api(handles[i].u.handle);
  }
  while (!try_stop_xapp_api()) {
    usleep(1000);
  }
  return handle_count == 0 ? 1 : 0;
}

/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 *
 * oranlab_kpm — long-running E2SM-KPM collector for the O-RAN lab.
 *
 * Subscribes to every KPM report style this program understands on RAN
 * Function 2 of each connected E2 node:
 *   - Style 1 (Action Definition Format 1): E2 node / cell-level measurements
 *   - Style 4 (Action Definition Format 4): UE-level measurements for every UE
 *     matching S-NSSAI SST == ORANLAB_KPM_SST (Indication Message Format 3)
 *
 * Every measurement report is printed to stdout as one JSON line so that a
 * separate process (monitoring/exporters/kpm_exporter.py) can turn it into
 * Prometheus metrics without parsing human-oriented logs.  Values are printed
 * exactly as received from the E2 node; unit conversion happens downstream.
 *
 * Environment:
 *   ORANLAB_KPM_PERIOD_MS  report period and granularity period (default 1000)
 *   ORANLAB_KPM_SST        S-NSSAI SST used by the Style 4 condition (default 1)
 *
 * Stops on SIGINT/SIGTERM and removes its subscriptions before exiting.
 */

#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/e.h"

#include <errno.h>
#include <inttypes.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

enum { KPM_RAN_FUNCTION = 2, MAX_HANDLES = 16 };

static volatile sig_atomic_t stop_requested = 0;
static pthread_mutex_t output_lock = PTHREAD_MUTEX_INITIALIZER;
static uint64_t sequence = 0;

static void request_stop(int signal_number)
{
  (void)signal_number;
  stop_requested = 1;
}

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
    fprintf(stderr, "ORANLAB_KPM_ERROR invalid %s=%s (expected %ld..%ld)\n",
            name, value, minimum, maximum);
    exit(EXIT_FAILURE);
  }
  return parsed;
}

static int64_t wall_clock_ms(void)
{
  struct timespec now;
  clock_gettime(CLOCK_REALTIME, &now);
  return (int64_t)now.tv_sec * 1000 + now.tv_nsec / 1000000;
}

/* Measurement names are printable ASCII identifiers such as "DRB.UEThpDl";
 * escape defensively anyway so a malformed name can never break the JSON. */
static void print_json_string(const byte_array_t name)
{
  putchar('"');
  for (size_t i = 0; i < name.len; ++i) {
    const unsigned char c = name.buf[i];
    if (c == '"' || c == '\\') {
      putchar('\\');
      putchar(c);
    } else if (c < 0x20 || c > 0x7e) {
      printf("\\u%04x", c);
    } else {
      putchar(c);
    }
  }
  putchar('"');
}

static void print_record(const meas_record_lst_t* record)
{
  if (record->value == INTEGER_MEAS_VALUE) {
    printf("%" PRIu32, record->int_val);
  } else if (record->value == REAL_MEAS_VALUE) {
    printf("%.6g", record->real_val);
  } else {
    printf("null");
  }
}

/* Format 1 message: meas_info_lst names the measurements, meas_data_lst holds
 * one record list per granularity period.  Records are consumed sequentially
 * across all (measurement, label) pairs, as in the KPM v3 encoding. */
static void print_measurements(const kpm_ind_msg_format_1_t* msg)
{
  printf("\"periods\":[");
  for (size_t d = 0; d < msg->meas_data_lst_len; ++d) {
    const meas_data_lst_t* data = &msg->meas_data_lst[d];
    size_t record = 0;
    bool first = true;
    printf("%s{", d == 0 ? "" : ",");
    for (size_t i = 0; i < msg->meas_info_lst_len; ++i) {
      const meas_info_format_1_lst_t* info = &msg->meas_info_lst[i];
      for (size_t z = 0; z < info->label_info_lst_len && record < data->meas_record_len; ++z) {
        const meas_record_lst_t* value = &data->meas_record_lst[record++];
        if (info->meas_type.type != NAME_MEAS_TYPE) {
          continue;
        }
        printf("%s", first ? "" : ",");
        first = false;
        print_json_string(info->meas_type.name);
        putchar(':');
        print_record(value);
      }
    }
    printf("}");
  }
  printf("]");
}

static void print_ue_id(const ue_id_e2sm_t* ue)
{
  printf("\"ue_id_type\":%d", (int)ue->type);
  if (ue->type == GNB_DU_UE_ID_E2SM) {
    printf(",\"gnb_cu_ue_f1ap_id\":%" PRIu32, ue->gnb_du.gnb_cu_ue_f1ap);
  }
}

static void sm_cb_kpm(sm_ag_if_rd_t const* rd)
{
  if (rd == NULL || rd->type != INDICATION_MSG_AGENT_IF_ANS_V0 ||
      rd->ind.type != KPM_STATS_V3_0) {
    return;
  }
  const kpm_ind_data_t* ind = &rd->ind.kpm.ind;
  const int64_t received_ms = wall_clock_ms();

  pthread_mutex_lock(&output_lock);
  if (ind->msg.type == FORMAT_1_INDICATION_MESSAGE) {
    printf("{\"type\":\"kpm\",\"seq\":%" PRIu64 ",\"ts_ms\":%" PRId64 ",\"scope\":\"cell\",",
           sequence++, received_ms);
    print_measurements(&ind->msg.frm_1);
    printf("}\n");
  } else if (ind->msg.type == FORMAT_3_INDICATION_MESSAGE) {
    const uint64_t indication = sequence++;
    for (size_t i = 0; i < ind->msg.frm_3.ue_meas_report_lst_len; ++i) {
      const meas_report_per_ue_t* ue = &ind->msg.frm_3.meas_report_per_ue[i];
      printf("{\"type\":\"kpm\",\"seq\":%" PRIu64 ",\"ts_ms\":%" PRId64 ",\"scope\":\"ue\",",
             indication, received_ms);
      print_ue_id(&ue->ue_meas_report_lst);
      putchar(',');
      print_measurements(&ue->ind_msg_format_1);
      printf("}\n");
    }
  } else {
    printf("{\"type\":\"unsupported\",\"ts_ms\":%" PRId64 ",\"msg_format\":%d}\n",
           received_ms, (int)ind->msg.type);
  }
  fflush(stdout);
  pthread_mutex_unlock(&output_lock);
}

static label_info_lst_t no_label(void)
{
  label_info_lst_t label = {0};
  label.noLabel = ecalloc(1, sizeof(enum_value_e));
  *label.noLabel = TRUE_ENUM_VALUE;
  return label;
}

static kpm_act_def_format_1_t fill_act_def_frm_1(const ric_report_style_item_t* style,
                                                 uint32_t period_ms)
{
  kpm_act_def_format_1_t ad = {0};
  ad.meas_info_lst_len = style->meas_info_for_action_lst_len;
  ad.meas_info_lst = ecalloc(ad.meas_info_lst_len, sizeof(meas_info_format_1_lst_t));
  for (size_t i = 0; i < ad.meas_info_lst_len; ++i) {
    ad.meas_info_lst[i].meas_type.type = NAME_MEAS_TYPE;
    ad.meas_info_lst[i].meas_type.name = copy_byte_array(style->meas_info_for_action_lst[i].name);
    ad.meas_info_lst[i].label_info_lst_len = 1;
    ad.meas_info_lst[i].label_info_lst = ecalloc(1, sizeof(label_info_lst_t));
    ad.meas_info_lst[i].label_info_lst[0] = no_label();
  }
  ad.gran_period_ms = period_ms;
  return ad;
}

static test_info_lst_t nssai_equals(uint8_t sst)
{
  test_info_lst_t dst = {0};
  dst.test_cond_type = S_NSSAI_TEST_COND_TYPE;
  dst.S_NSSAI = TRUE_TEST_COND_TYPE;
  dst.test_cond = ecalloc(1, sizeof(test_cond_e));
  *dst.test_cond = EQUAL_TEST_COND;
  dst.test_cond_value = ecalloc(1, sizeof(test_cond_value_t));
  dst.test_cond_value->type = OCTET_STRING_TEST_COND_VALUE;
  dst.test_cond_value->octet_string_value = ecalloc(1, sizeof(byte_array_t));
  dst.test_cond_value->octet_string_value->len = 1;
  dst.test_cond_value->octet_string_value->buf = ecalloc(1, sizeof(uint8_t));
  dst.test_cond_value->octet_string_value->buf[0] = sst;
  return dst;
}

static bool make_subscription(const ric_report_style_item_t* style,
                              uint32_t period_ms,
                              uint8_t sst,
                              kpm_sub_data_t* sub)
{
  memset(sub, 0, sizeof(*sub));
  sub->ev_trg_def.type = FORMAT_1_RIC_EVENT_TRIGGER;
  sub->ev_trg_def.kpm_ric_event_trigger_format_1.report_period_ms = period_ms;
  sub->sz_ad = 1;
  sub->ad = ecalloc(1, sizeof(kpm_act_def_t));

  if (style->act_def_format_type == FORMAT_1_ACTION_DEFINITION) {
    sub->ad->type = FORMAT_1_ACTION_DEFINITION;
    sub->ad->frm_1 = fill_act_def_frm_1(style, period_ms);
    return true;
  }
  if (style->act_def_format_type == FORMAT_4_ACTION_DEFINITION) {
    sub->ad->type = FORMAT_4_ACTION_DEFINITION;
    sub->ad->frm_4.matching_cond_lst_len = 1;
    sub->ad->frm_4.matching_cond_lst = ecalloc(1, sizeof(matching_condition_format_4_lst_t));
    sub->ad->frm_4.matching_cond_lst[0].test_info_lst = nssai_equals(sst);
    sub->ad->frm_4.action_def_format_1 = fill_act_def_frm_1(style, period_ms);
    return true;
  }
  free(sub->ad);
  sub->ad = NULL;
  sub->sz_ad = 0;
  return false;
}

static size_t find_rf(const e2_node_connected_xapp_t* node, int id)
{
  for (size_t i = 0; i < node->len_rf; ++i) {
    if (node->rf[i].id == (uint16_t)id) {
      return i;
    }
  }
  return node->len_rf;
}

int main(int argc, char* argv[])
{
  const uint32_t period_ms = (uint32_t)env_long("ORANLAB_KPM_PERIOD_MS", 1000, 100, 60000);
  const uint8_t sst = (uint8_t)env_long("ORANLAB_KPM_SST", 1, 0, 255);

  struct sigaction action = {0};
  action.sa_handler = request_stop;
  sigemptyset(&action.sa_mask);
  sigaction(SIGINT, &action, NULL);
  sigaction(SIGTERM, &action, NULL);

  fr_args_t args = init_fr_args(argc, argv);
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  sm_ans_xapp_t handles[MAX_HANDLES] = {0};
  size_t handle_count = 0;

  for (size_t n = 0; n < nodes.len; ++n) {
    e2_node_connected_xapp_t* node = &nodes.n[n];
    const size_t index = find_rf(node, KPM_RAN_FUNCTION);
    if (index >= node->len_rf || node->rf[index].defn.type != KPM_RAN_FUNC_DEF_E) {
      continue;
    }
    const kpm_ran_function_def_t* kpm = &node->rf[index].defn.kpm;
    for (size_t s = 0; s < kpm->sz_ric_report_style_list && handle_count < MAX_HANDLES; ++s) {
      const ric_report_style_item_t* style = &kpm->ric_report_style_list[s];
      kpm_sub_data_t sub;
      if (!make_subscription(style, period_ms, sst, &sub)) {
        continue;
      }
      sm_ans_xapp_t answer = report_sm_xapp_api(&node->id, KPM_RAN_FUNCTION, &sub, sm_cb_kpm);
      free_kpm_sub_data(&sub);
      pthread_mutex_lock(&output_lock);
      printf("{\"type\":\"subscription\",\"ts_ms\":%" PRId64 ",\"node\":%zu,"
             "\"report_style\":%d,\"action_definition_format\":%d,"
             "\"measurements\":%zu,\"period_ms\":%" PRIu32 ",\"success\":%s}\n",
             wall_clock_ms(), n, (int)style->report_style_type + 1,
             (int)style->act_def_format_type + 1, style->meas_info_for_action_lst_len,
             period_ms, answer.success ? "true" : "false");
      fflush(stdout);
      pthread_mutex_unlock(&output_lock);
      if (answer.success) {
        handles[handle_count++] = answer;
      }
    }
  }
  free_e2_node_arr_xapp(&nodes);

  int exit_code = EXIT_SUCCESS;
  if (handle_count == 0) {
    fprintf(stderr, "ORANLAB_KPM_ERROR no KPM subscription was accepted\n");
    exit_code = EXIT_FAILURE;
  } else {
    while (!stop_requested) {
      usleep(100000);
    }
  }

  /* Subscription Delete waits for the RIC's answer.  If the RIC is already
   * gone that wait never ends, so bound the whole shutdown: SIGALRM's default
   * action terminates the process. */
  alarm(3);
  for (size_t i = 0; i < handle_count; ++i) {
    rm_report_sm_xapp_api(handles[i].u.handle);
  }
  while (!try_stop_xapp_api()) {
    usleep(1000);
  }
  alarm(0);
  return exit_code;
}

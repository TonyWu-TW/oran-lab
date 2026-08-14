/*
 * e2_hello — FlexRIC xApp API 第 0 步
 *
 * 只做三件事：連上 Near-RT RIC、列出已連線的 E2 node、列出每個 node
 * 廣告的 RAN Function ID。還沒有訂閱、也還沒有下控制。
 *
 * 對應 FlexRIC 公開 API（全部在 e42_xapp_api.h）：
 *   init_xapp_api()      連 RIC
 *   e2_nodes_xapp_api()  看現在有誰、支援哪些 SM
 *   try_stop_xapp_api()  離開
 */

#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/alg_ds/alg/defer.h"
#include "../../../../src/util/ngran_types.h"

#include <stdio.h>
#include <unistd.h>

static const char* ran_function_name(uint16_t id)
{
  /* FlexRIC / OCUDU 慣例：2 = E2SM-KPM（觀測），3 = E2SM-RC（控制）。
   * 真正要以 e2_nodes_xapp_api() 回傳的 rf[].id 為準。 */
  switch (id) {
    case 2: return "E2SM-KPM (subscribe / indication)";
    case 3: return "E2SM-RC  (control)";
    default: return "custom or unknown SM";
  }
}

int main(int argc, char* argv[])
{
  fr_args_t args = init_fr_args(argc, argv);
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  defer({ free_e2_node_arr_xapp(&nodes); });

  printf("Connected E2 nodes = %d\n", nodes.len);
  if (nodes.len == 0) {
    printf("No E2 node. Start nearRT-RIC and the gNB first.\n");
  }

  for (size_t i = 0; i < nodes.len; ++i) {
    const e2_node_connected_xapp_t* node = &nodes.n[i];
    printf("\nE2 node %zu  nb_id=%d  plmn=%d%02d  type=%s  ran_functions=%zu\n",
           i,
           node->id.nb_id.nb_id,
           node->id.plmn.mcc,
           node->id.plmn.mnc,
           get_ngran_name(node->id.type),
           node->len_rf);

    for (size_t j = 0; j < node->len_rf; ++j) {
      const uint16_t id = node->rf[j].id;
      printf("  RAN Function %u  %s\n", id, ran_function_name(id));
    }
  }

  while (!try_stop_xapp_api()) {
    usleep(1000);
  }
  return 0;
}

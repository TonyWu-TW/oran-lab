/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "sqlite3_wrapper.h"
#include "../../../util/time_now_us.h"
#include "../../../util/e.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include "string.h"

#include "NR_UL-DCCH-Message.h"
#include "../../../lib/sm/dec/dec_ue_id.h"

#define MAX_SQL_LENGTH 512

typedef struct {
  const char* name;
  const char* unit;
} measure_info_t;

typedef struct {
  char id[128];
  char group[64];
  char ran_ue_id[128];
} ue_id_info_t;

static
void create_table(sqlite3* db, char* sql)
{
  char* err_msg = NULL;
  int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  if (rc != SQLITE_OK) {
    printf("SQLite database creation failed: %s.\n", err_msg);
    assert(false);
  }
}

static
void create_mac_ue_table(sqlite3* db)
{
  assert(db != NULL);

  // ToDo: PRIMARY KEY UNIQUE
  char* sql_mac = "DROP TABLE IF EXISTS MAC_UE;"
  "CREATE TABLE MAC_UE(tstamp INT CHECK(tstamp > 0)," 
                       "ngran_node INT CHECK(ngran_node >= 0 AND ngran_node < 9),"
                       "mcc INT,"
                       "mnc INT,"
                       "mnc_digit_len INT,"
                       "nb_id INT,"
                       "cu_du_id TEXT,"
                       "frame INT,"
                       "slot INT,"
                       "dl_aggr_tbs INT CHECK(dl_aggr_tbs  >= 0 AND dl_aggr_tbs < 18446744073709551615),"
                       "ul_aggr_tbs INT CHECK(ul_aggr_tbs  >= 0 AND ul_aggr_tbs < 18446744073709551615),"
                       "dl_aggr_bytes_sdus  INT CHECK(dl_aggr_bytes_sdus  >= 0 AND dl_aggr_bytes_sdus < 18446744073709551615),"
                       "ul_aggr_bytes_sdus INT CHECK(ul_aggr_bytes_sdus >= 0 AND ul_aggr_bytes_sdus < 18446744073709551615),"
                       "dl_curr_tbs INT CHECK(dl_curr_tbs >= 0 AND dl_curr_tbs < 18446744073709551615),"
                       "ul_curr_tbs INT CHECK(ul_curr_tbs >= 0 AND ul_curr_tbs < 18446744073709551615),"
                       "dl_sched_rb INT CHECK(dl_sched_rb >= 0 AND dl_sched_rb < 18446744073709551615),"
                       "ul_sched_rb INT CHECK(ul_sched_rb >= 0 AND ul_sched_rb < 18446744073709551615),"
                       "pusch_snr REAL CHECK(pusch_snr == NULL OR (pusch_snr >= 0 AND pusch_snr < 4294967296)),"
                       "pucch_snr REAL CHECK(pucch_snr == NULL OR (pucch_snr >= 0 AND pucch_snr < 4294967296)) ,"
                       "rnti INT  CHECK(rnti  >= 0 AND rnti < 4294967296 ),"
                       "dl_aggr_prb INT  CHECK(dl_aggr_prb >= 0 AND dl_aggr_prb < 4294967296 ),"
                       "ul_aggr_prb INT  CHECK(ul_aggr_prb >= 0 AND ul_aggr_prb < 4294967296 ),"
                       "dl_aggr_sdus INT  CHECK(dl_aggr_sdus >= 0 AND dl_aggr_sdus < 4294967296 ),"
                       "ul_aggr_sdus INT  CHECK(ul_aggr_sdus >= 0 AND ul_aggr_sdus < 4294967296 ),"
                       "dl_aggr_retx_prb  INT  CHECK(dl_aggr_retx_prb >= 0 AND dl_aggr_retx_prb < 4294967296 ),"
                       "ul_aggr_retx_prb  INT  CHECK(ul_aggr_retx_prb >= 0 AND ul_aggr_retx_prb < 4294967296 ),"
                       "wb_cqi INT  CHECK(wb_cqi >= 0 AND wb_cqi < 256 ),"
                       "dl_mcs1 INT  CHECK(dl_mcs1>= 0 AND dl_mcs1 < 256),"
                       "ul_mcs1  INT CHECK(ul_mcs1 >= 0 AND ul_mcs1 < 256),"
                       "dl_mcs2  INT CHECK(dl_mcs2 >= 0 AND dl_mcs2 < 256),"
                       "ul_mcs2 INT CHECK(ul_mcs2 >= 0 AND ul_mcs2 < 256),"
                       "phr INT CHECK(phr >= -128 AND  phr < 128),"
                       "bsr INT CHECK(bsr >= 0 AND  bsr < 4294967296),"
                       "dl_bler REAL CHECK(dl_bler  >= 0 AND dl_bler < 4294967296),"
                       "ul_bler REAL CHECK(ul_bler  >= 0 AND ul_bler < 4294967296),"
                       "dl_num_harq INT CHECK(dl_num_harq >= 0 AND  dl_num_harq < 5),"
                       "dl_harq_round0 INT CHECK(dl_harq_round0 >= 0 AND  dl_harq_round0 < 4294967296),"
                       "dl_harq_round1 INT CHECK(dl_harq_round1 >= 0 AND  dl_harq_round1 < 4294967296),"
                       "dl_harq_round2 INT CHECK(dl_harq_round2 >= 0 AND  dl_harq_round2 < 4294967296),"
                       "dl_harq_round3 INT CHECK(dl_harq_round3 >= 0 AND  dl_harq_round3 < 4294967296),"
                       "dlsch_errors INT CHECK(dlsch_errors >= 0 AND  dlsch_errors < 4294967296),"
                       "ul_num_harq INT CHECK(ul_num_harq >= 0 AND  ul_num_harq < 5),"
                       "ul_harq_round0 INT CHECK(ul_harq_round0 >= 0 AND  ul_harq_round0 < 4294967296),"
                       "ul_harq_round1 INT CHECK(ul_harq_round1 >= 0 AND  ul_harq_round1 < 4294967296),"
                       "ul_harq_round2 INT CHECK(ul_harq_round2 >= 0 AND  ul_harq_round2 < 4294967296),"
                       "ul_harq_round3 INT CHECK(ul_harq_round3 >= 0 AND  ul_harq_round3 < 4294967296),"
                       "ulsch_errors INT CHECK(ulsch_errors >= 0 AND  ulsch_errors < 4294967296)"
                       ");";

  create_table(db, sql_mac);
}

static
void create_rlc_bearer_table(sqlite3* db)
{
  assert(db != NULL);

  // ToDo: PRIMARY KEY UNIQUE
  char* sql_rlc = "DROP TABLE IF EXISTS RLC_bearer;"
  "CREATE TABLE RLC_bearer(tstamp INT CHECK(tstamp > 0)," 
                            "ngran_node INT CHECK(ngran_node >= 0 AND ngran_node < 9),"
                            "mcc INT,"
                            "mnc INT,"
                            "mnc_digit_len INT,"
                            "nb_id INT,"
                            "cu_du_id TEXT,"
                            "txpdu_pkts INT CHECK(txpdu_pkts >= 0 AND txpdu_pkts < 4294967296)," // 1 << 32 = 4294967296 
                            "txpdu_bytes INT CHECK(txpdu_bytes >= 0 AND  txpdu_bytes < 4294967296 ),"\
                            "txpdu_wt_ms  INT CHECK(txpdu_wt_ms  >= 0 AND  txpdu_wt_ms  < 4294967296 ),"\
                            "txpdu_dd_pkts INT CHECK(txpdu_dd_pkts >= 0 AND  txpdu_dd_pkts < 4294967296 ) ,"\
                            "txpdu_dd_bytes INT  CHECK(txpdu_dd_bytes >= 0 AND  txpdu_dd_bytes < 4294967296 ),"\
                            "txpdu_retx_pkts INT CHECK(txpdu_retx_pkts >= 0 AND  txpdu_retx_pkts < 4294967296 ) ,"\
                            "txpdu_retx_bytes INT  CHECK(txpdu_retx_bytes >= 0 AND  txpdu_retx_bytes < 4294967296 ),"\
                            "txpdu_segmented INT  CHECK(txpdu_segmented >= 0 AND  txpdu_segmented < 4294967296 ),"\
                            "txpdu_status_pkts INT  CHECK(txpdu_status_pkts >= 0 AND  txpdu_status_pkts < 4294967296 ),"\
                            "txpdu_status_bytes INT  CHECK(txpdu_status_bytes >= 0 AND  txpdu_status_bytes < 4294967296 ),"\
                            "txbuf_occ_bytes INT  CHECK(txbuf_occ_bytes >= 0 AND  txbuf_occ_bytes < 4294967296 ),"\
                            "txbuf_occ_pkts INT  CHECK(txbuf_occ_pkts >= 0 AND  txbuf_occ_pkts < 4294967296 ),"\
                            "rxpdu_pkts INT  CHECK(rxpdu_pkts >= 0 AND  rxpdu_pkts < 4294967296 ),"\
                            "rxpdu_bytes INT  CHECK(rxpdu_bytes >= 0 AND  rxpdu_bytes < 4294967296 ),"\
                            "rxpdu_dup_pkts INT CHECK(rxpdu_dup_pkts >= 0 AND  rxpdu_dup_pkts < 4294967296 ),"\
                            "rxpdu_dup_bytes INT CHECK(rxpdu_dup_bytes >= 0 AND  rxpdu_dup_bytes < 4294967296 ),"\
                            "rxpdu_dd_pkts INT CHECK(rxpdu_dd_pkts >= 0 AND  rxpdu_dd_pkts < 4294967296 ),"\
                            "rxpdu_dd_bytes INT CHECK(rxpdu_dd_bytes >= 0 AND  rxpdu_dd_bytes < 4294967296 ),"\
                            "rxpdu_ow_pkts INT CHECK(rxpdu_ow_pkts >= 0 AND  rxpdu_ow_pkts < 4294967296 ),"\
                            "rxpdu_ow_bytes INT CHECK(rxpdu_ow_bytes >= 0 AND  rxpdu_ow_bytes < 4294967296 ),"\
                            "rxpdu_status_pkts INT CHECK(rxpdu_status_pkts >= 0 AND  rxpdu_status_pkts < 4294967296 ),"\
                            "rxpdu_status_bytes INT CHECK(rxpdu_status_bytes >= 0 AND  rxpdu_status_bytes < 4294967296 ),"\
                            "rxbuf_occ_bytes INT CHECK(rxbuf_occ_bytes >= 0 AND  rxbuf_occ_bytes < 4294967296 ),"\
                            "rxbuf_occ_pkts INT CHECK(rxbuf_occ_pkts >= 0 AND  rxbuf_occ_pkts < 4294967296 ),"\
                            "txsdu_pkts INT CHECK(txsdu_pkts >= 0 AND  txsdu_pkts < 4294967296 ),"\
                            "txsdu_bytes INT CHECK(txsdu_bytes >= 0 AND  txsdu_bytes < 4294967296 ),"\
                            "txsdu_avg_time_to_tx REAL CHECK(txsdu_avg_time_to_tx >= 0 AND  txsdu_avg_time_to_tx < 4294967296 ),"\
                            "txsdu_wt_us INT CHECK(txsdu_wt_us >= 0 AND  txsdu_wt_us < 4294967296 ),"\
                            "rxsdu_pkts INT CHECK(rxsdu_pkts >= 0 AND  rxsdu_pkts < 4294967296 ),"\
                            "rxsdu_bytes INT CHECK(rxsdu_bytes >= 0 AND  rxsdu_bytes < 4294967296 ),"\
                            "rxsdu_dd_pkts INT CHECK(rxsdu_dd_pkts >= 0 AND  rxsdu_dd_pkts < 4294967296 ),"\
                            "rxsdu_dd_bytes INT CHECK(rxsdu_dd_bytes >= 0 AND  rxsdu_dd_bytes < 4294967296 ),"\
                            "rnti INT CHECK(rnti >= 0 AND  rnti < 4294967296 ),"\
                            "mode INT CHECK(mode >= 0 AND  mode < 3),"\
                            "rbid INT CHECK(rbid >= 0 AND  rbid < 16)"
                            ");";

  create_table(db, sql_rlc);
}

static
void create_pdcp_bearer_table(sqlite3* db)
{
  assert(db != NULL);

  // ToDo: PRIMARY KEY UNIQUE
  char* sql_pdcp = "DROP TABLE IF EXISTS PDCP_bearer;"
  "CREATE TABLE PDCP_bearer(tstamp INT CHECK(tstamp > 0)," 
                       "ngran_node INT CHECK(ngran_node >= 0 AND ngran_node <= 10),"
                       "mcc INT,"
                       "mnc INT,"
                       "mnc_digit_len INT,"
                       "nb_id INT,"
                       "cu_du_id TEXT,"
                       "txpdu_pkts INT CHECK(txpdu_pkts  >= 0 AND txpdu_pkts  < 4294967296),"
                       "txpdu_bytes INT CHECK(txpdu_bytes >=0 AND txpdu_bytes < 4294967296 ),"
                       "txpdu_sn INT CHECK(txpdu_sn >=0 AND txpdu_sn < 4294967296 ),"
                       "rxpdu_pkts INT CHECK(rxpdu_pkts >=0 AND rxpdu_pkts < 4294967296 ),"
                       "rxpdu_bytes INT CHECK(rxpdu_bytes >=0 AND rxpdu_bytes < 4294967296 ),"
                       "rxpdu_sn  INT CHECK(rxpdu_sn  >= 0 AND rxpdu_sn  < 4294967296 ) ,"
                       "rxpdu_oo_pkts INT  CHECK(rxpdu_oo_pkts >= 0 AND rxpdu_oo_pkts < 4294967296 ),"
                       "rxpdu_oo_bytes INT  CHECK(rxpdu_oo_bytes >= 0 AND rxpdu_oo_bytes < 4294967296 ),"
                       "rxpdu_dd_pkts INT  CHECK(rxpdu_dd_pkts >= 0 AND rxpdu_dd_pkts < 4294967296 ),"
                       "rxpdu_dd_bytes INT  CHECK(rxpdu_dd_bytes >= 0 AND rxpdu_dd_bytes < 4294967296 ),"
                       "rxpdu_ro_count INT  CHECK(rxpdu_ro_count >= 0 AND rxpdu_ro_count < 4294967296 ),"
                       "txsdu_pkts INT  CHECK(txsdu_pkts >= 0 AND txsdu_pkts < 4294967296 ),"
                       "txsdu_bytes INT  CHECK(txsdu_bytes >= 0 AND txsdu_bytes <4294967296 ),"
                       "rxsdu_pkts INT  CHECK(rxsdu_pkts >= 0 AND rxsdu_pkts <4294967296),"
                       "rxsdu_bytes INT CHECK(rxsdu_bytes >= 0 AND rxsdu_bytes <4294967296),"
                       "rnti INT CHECK(rnti >= 0 AND rnti <4294967296),"
                       "mode INT CHECK(mode >= 0 AND mode < 4294967296),"
                       "rbid INT CHECK(rbid >= 0 AND rbid < 4294967296)" // −23 dB to +40 dB 
                       ");";
  create_table(db, sql_pdcp);
}

static
void create_slice_table(sqlite3* db)
{
  assert(db != NULL);

  // ToDo: PRIMARY KEY UNIQUE
  char* sql_slice = "DROP TABLE IF EXISTS SLICE;"
                    "CREATE TABLE SLICE("\
                    "tstamp INT CHECK(tstamp > 0),"\
                    "ngran_node INT CHECK(ngran_node >= 0 AND ngran_node < 9),"\
                    "mcc INT,"\
                    "mnc INT,"\
                    "mnc_digit_len INT,"\
                    "nb_id INT,"\
                    "cu_du_id TEXT,"\
                    "len_slices INT CHECK(len_slices  >= 0 AND len_slices < 4),"\
                    "sched_name TEXT,"\
                    "id INT CHECK(id >=0 AND id < 4294967296),"\
                    "label TEXT,"\
                    "type TEXT,"\
                    "type_conf TEXT,"\
                    "sched TEXT,"\
                    "type_param0 REAL CHECK(type_param0 == NULL OR (type_param0 >= 0 AND type_param0 < 4294967296)),"\
                    "type_param1 REAL CHECK(type_param1 == NULL OR (type_param1 >= 0 AND type_param1 < 4294967296)),"\
                    "type_param2 REAL CHECK(type_param2 == NULL OR (type_param2 >= 0 AND type_param2 < 4294967296))"
                    ");";
  create_table(db, sql_slice);
}

static
void create_ue_slice_table(sqlite3* db)
{
  assert(db != NULL);

  // ToDo: PRIMARY KEY UNIQUE
  char* sql_ue_slice = "DROP TABLE IF EXISTS UE_SLICE;"
                    "CREATE TABLE UE_SLICE("\
                    "tstamp INT CHECK(tstamp > 0),"\
                    "ngran_node INT CHECK(ngran_node >= 0 AND ngran_node < 9),"\
                    "mcc INT,"\
                    "mnc INT,"\
                    "mnc_digit_len INT,"\
                    "nb_id INT,"\
                    "cu_du_id TEXT,"\
                    "len_ue_slice INT CHECK(len_ue_slice  >= 0 AND len_ue_slice  < 4294967296),"\
                    "rnti INT CHECK(rnti == -1 OR (rnti >= 0 AND rnti < 65535)),"\
                    "dl_id INT CHECK(dl_id == -1 OR (dl_id >= 0 AND dl_id < 4294967296))"
                    ");";
  create_table(db, sql_ue_slice);
}

static
void create_gtp_table(sqlite3* db)
{
  assert(db != NULL);

  // ToDo: PRIMARY KEY UNIQUE
  char* sql_gtp = "DROP TABLE IF EXISTS GTP_NGUT;"
  "CREATE TABLE GTP_NGUT(tstamp INT CHECK(tstamp > 0)," 
                            "ngran_node INT CHECK(ngran_node >= 0 AND ngran_node <= 10),"
                            "mcc INT,"
                            "mnc INT,"
                            "mnc_digit_len INT,"
                            "nb_id INT,"
                            "cu_du_id TEXT,"
                            "teidgnb INT ," // 1 << 32 = 4294967296 
                            "rnti INT ,"
                            "qfi  INT ,"
                            "teidupf INT "
                            ");";

  create_table(db, sql_gtp);
}

static
void create_kpm_table(sqlite3* db) 
{
  assert(db != NULL);

  char* sql_kpm_table = "DROP TABLE IF EXISTS KPM_MeasRecord;"
  "CREATE TABLE KPM_MeasRecord(tstamp INT CHECK(tstamp > 0),"
                               "ngran_type TEXT,"
                               "nb_id INT,"
                               "mcc INT,"
                               "mnc INT,"
                               "cu_du_id INT,"
                               "indication_message_format INT,"
                               "ue_id TEXT,"
                               "ue_id_group TEXT,"
                               "ran_ue_id TEXT,"
                               "meas_name TEXT,"
                               "binX INT,"
                               "binY INT,"
                               "binZ INT,"
                               "measure_value REAL,"
                               "incompleteFlag TEXT"
                               ");";

  create_table(db, sql_kpm_table);
}

static
void create_rc_meas_report_table(sqlite3* db)
{
  assert(db != NULL);
  char* sql_rc_meas = "DROP TABLE IF EXISTS RC_MEAS_REPORT;"
                      "CREATE TABLE RC_MEAS_REPORT("
                      "  tstamp INT CHECK(tstamp > 0),"
                      "  ngran_type TEXT,"
                      "  nb_id INT,"
                      "  mcc INT,"
                      "  mnc INT,"
                      "  cu_du_id INT,"
                      "  ran_ue_id TEXT,"
                      "  rsrp INT,"
                      "  rsrq REAL,"
                      "  sinr REAL"
                      ");";
  create_table(db, sql_rc_meas);
}

static
void insert_db(sqlite3* db, char const* sql)
{
  assert(db != NULL);
  assert(sql != NULL);

  char* err_msg = NULL;
  int rc = sqlite3_exec(db, sql, 0, 0, &err_msg);
  assert(rc == SQLITE_OK && "Error while inserting into the DB. Check the err_msg string for further info");
}


static
int to_sql_string_mac_ue(global_e2_node_id_t const* id, mac_ue_stats_impl_t* stats, int64_t tstamp, char* out, size_t out_len)
{
  assert(stats != NULL);       
  assert(out != NULL);
  const size_t max = 1024;
  assert(out_len >= max);

  char* c_null = NULL;
  char c_cu_du_id[26];
  if (id->cu_du_id) {
    int rc = snprintf(c_cu_du_id, 26, "%lu", *id->cu_du_id);
    assert(rc < (int) max && "Not enough space in the char array to write all the data");
  }

  int rc = snprintf(out, max, 
      "INSERT INTO MAC_UE VALUES("
      "%ld,"//tstamp
      "%d," //ngran_node  
      "%d," //mcc
      "%d," //mnc
      "%d," //mnc_digit_len   
      "%d," //nb_id 
      "'%s'," //cu_du_id
      "%d," //frame
      "%d," //slot
      "%lu,"//dl_aggr_tbs     
      "%lu,"//ul_aggr_tbs     
      "%lu," //dl_aggr_bytes_sdus   
      "%lu,"//ul_aggr_bytes_sdus       
      "%lu," //dl_curr_tbs
      "%lu," //ul_curr_tbs
      "%lu," //dl_sched_rb
      "%lu," //ul_sched_rb
      "%g,"// pusch_snr        
      "%g,"//  pucch_snr      
      "%u,"// rnti            
      "%u,"//dl_aggr_prb      
      "%u,"//  ul_aggr_prb    
      "%u,"//  dl_aggr_sdus   
      "%u,"//  ul_aggr_sdus   
      "%u,"// dl_aggr_retx_prb
      "%u,"// ul_aggr_retx_prb
      "%u,"// wb_cqi
      "%u,"// dl_mcs1
      "%u,"// ul_mcs1
      "%u,"// dl_mcs2         
      "%u,"// ul_mcs2         
      "%d,"// phr
      "%u,"// bsr
      "%f," // dl_bler
      "%f," // ul_bler
      "%d," // dl_num_harq
      "%u," // dl_harq[0]
      "%u," // dl_harq[1]
      "%u," // dl_harq[2]
      "%u," // dl_harq[3]
      "%u," // dlsch_errors
      "%d," // ul_num_harq
      "%u," // ul_harq[0]
      "%u," // ul_harq[1]
      "%u," // ul_harq[2]
      "%u," // ul_harq[3]
      "%u" // ulsch_errors
      ");"
      ,tstamp                
      ,id->type
      ,id->plmn.mcc
      ,id->plmn.mnc
      ,id->plmn.mnc_digit_len
      ,id->nb_id.nb_id 
      ,id->cu_du_id ? c_cu_du_id : c_null
      ,stats->frame
      ,stats->slot
      ,stats->dl_aggr_tbs    
      ,stats->ul_aggr_tbs    
      ,stats->dl_aggr_bytes_sdus      
      ,stats->ul_aggr_bytes_sdus      
      ,stats->dl_curr_tbs
      ,stats->ul_curr_tbs
      ,stats->dl_sched_rb
      ,stats->ul_sched_rb
      ,stats->pusch_snr      
      ,stats->pucch_snr      
      ,stats->rnti 
      ,stats->dl_aggr_prb    
      ,stats->ul_aggr_prb    
      ,stats->dl_aggr_sdus   
      ,stats->ul_aggr_sdus   
      ,stats->dl_aggr_retx_prb        
      ,stats->ul_aggr_retx_prb
      ,stats->wb_cqi         
      ,stats->dl_mcs1        
      ,stats->ul_mcs1        
      ,stats->dl_mcs2        
      ,stats->ul_mcs2        
      ,stats->phr            
      ,stats->bsr
      ,stats->dl_bler
      ,stats->ul_bler
      ,stats->dl_num_harq
      ,stats->dl_harq[0]
      ,stats->dl_harq[1]
      ,stats->dl_harq[2]
      ,stats->dl_harq[3]
      ,stats->dl_harq[4]
      ,stats->ul_num_harq
      ,stats->ul_harq[0]
      ,stats->ul_harq[1]
      ,stats->ul_harq[2]
      ,stats->ul_harq[3]
      ,stats->ul_harq[4]
      );
  assert(rc < (int)max && "Not enough space in the char array to write all the data");
  return rc;
}


static
int to_sql_string_rlc_rb(global_e2_node_id_t const* id,rlc_radio_bearer_stats_t* rlc, int64_t tstamp, char* out, size_t out_len)
{
  assert(rlc != NULL);
  assert(out != NULL);
  const size_t max = 1024;
  assert(out_len >= max);

  char* c_null = NULL;
  char c_cu_du_id[26];
  if (id->cu_du_id) {
    int rc = snprintf(c_cu_du_id, 26, "%lu", *id->cu_du_id);
    assert(rc < (int) max && "Not enough space in the char array to write all the data");
  }

  int const rc = snprintf(out, max,
        "INSERT INTO RLC_bearer VALUES("
        "%ld,"// tstamp
        "%d," //ngran_node  
        "%d," //mcc
        "%d," //mnc
        "%d," //mnc_digit_len   
        "%d," //nb_id 
        "'%s'," //cu_du_id
        "%u," //rlc->txpdu_pkts
        "%u," //rlc->txpdu_bytes
        "%u," //rlc->txpdu_wt_ms 
        "%u," //rlc->txpdu_dd_pkts
        "%u," //rlc->txpdu_dd_bytes 
        "%u," //rlc->txpdu_retx_pkts
        "%u," //rlc->txpdu_retx_bytes
        "%u," //rlc->txpdu_segmented
        "%u," //rlc->txpdu_status_pkts
        "%u," //rlc->txpdu_status_bytes
        "%u," //rlc->txbuf_occ_bytes
        "%u," //rlc->txbuf_occ_pkts
        "%u," //rlc->rxpdu_pkts
        "%u," //rlc->rxpdu_bytes
        "%u," //rlc->rxpdu_dup_pkts
        "%u," //rlc->rxpdu_dup_bytes
        "%u," //rlc->rxpdu_dd_pkts
        "%u," //rlc->rxpdu_dd_bytes
        "%u," //rlc->rxpdu_ow_pkts
        "%u," //rlc->rxpdu_ow_bytes
        "%u," //rlc->rxpdu_status_pkts
        "%u," //rlc->rxpdu_status_bytes
        "%u," //rlc->rxbuf_occ_bytes
        "%u," //rlc->rxbuf_occ_pkts
        "%u," //rlc->txsdu_pkts
        "%lu," //rlc->txsdu_bytes
        "%.2f," //rlc->txsdu_avg_time_to_tx
        "%u," //rlc->txsdu_wt_us
        "%u," //rlc->rxsdu_pkts
        "%lu," //rlc->rxsdu_bytes
        "%u," //rlc->rxsdu_dd_pkts
        "%u," //rlc->rxsdu_dd_bytes
        "%u," //rlc->rnti
        "%u,"  //rlc->mode
        "%u"  //rlc->rbid 
        ");"
        , tstamp
        , id->type
        , id->plmn.mcc
        , id->plmn.mnc
        , id->plmn.mnc_digit_len
        , id->nb_id.nb_id 
        , id->cu_du_id ? c_cu_du_id : c_null
        , rlc->txpdu_pkts
        , rlc->txpdu_bytes
        , rlc->txpdu_wt_ms
        , rlc->txpdu_dd_pkts
        , rlc->txpdu_dd_bytes
        , rlc->txpdu_retx_pkts
        , rlc->txpdu_retx_bytes
        , rlc->txpdu_segmented
        , rlc->txpdu_status_pkts
        , rlc->txpdu_status_bytes
        , rlc->txbuf_occ_bytes
        , rlc->txbuf_occ_pkts
        , rlc->rxpdu_pkts
        , rlc->rxpdu_bytes
        , rlc->rxpdu_dup_pkts
        , rlc->rxpdu_dup_bytes
        , rlc->rxpdu_dd_pkts
        , rlc->rxpdu_dd_bytes
        , rlc->rxpdu_ow_pkts
        , rlc->rxpdu_ow_bytes
        , rlc->rxpdu_status_pkts
        , rlc->rxpdu_status_bytes
        , rlc->rxbuf_occ_bytes
        , rlc->rxbuf_occ_pkts
        , rlc->txsdu_pkts
        , rlc->txsdu_bytes
        , rlc->txsdu_avg_time_to_tx
        , rlc->txsdu_wt_us
        , rlc->rxsdu_pkts
        , rlc->rxsdu_bytes
        , rlc->rxsdu_dd_pkts
        , rlc->rxsdu_dd_bytes
        , rlc->rnti
        , rlc->mode
        , rlc->rbid
        );
  assert(rc < (int)max && "Not enough space in the char array to write all the data");
  return rc;
}


static
int to_sql_string_pdcp_rb(global_e2_node_id_t const* id, pdcp_radio_bearer_stats_t* pdcp, int64_t tstamp, char* out, size_t out_len)
{                              
  assert(pdcp != NULL);        
  assert(out != NULL);
  const size_t max = 512;      
  assert(out_len >= max);

  char* c_null = NULL;
  char c_cu_du_id[26];
  if (id->cu_du_id) {
    int rc = snprintf(c_cu_du_id, 26, "%lu", *id->cu_du_id);
    assert(rc < (int) max && "Not enough space in the char array to write all the data");
  }

  int rc = snprintf(out, out_len, 
      "INSERT INTO PDCP_bearer VALUES("
        "%ld," //tstamp         
        "%d," //ngran_node  
        "%d," //mcc
        "%d," //mnc
        "%d," //mnc_digit_len   
        "%d," //nb_id 
        "'%s'," //cu_du_id
        "%u," //txpdu_pkts       
        "%u," //txpdu_bytes     
        "%u," // txpdu_sn   
        "%u," //rxpdu_pkts      
        "%u," //rxpdu_bytes     
        "%u,"     //rxpdu_sn    
        "%u," //rxpdu_oo_pkts   
        "%u," //rxpdu_oo_bytes  
        "%u,"   //rxpdu_dd_pkts 
        "%u,"  //rxpdu_dd_bytes 
        "%u," //rxpdu_ro_count  
        "%u,"//txsdu_pkts
        "%u," //txsdu_bytes     
        "%u,"//rxsdu_pkts
        "%u," //rxsdu_bytes     
        "%u," //rnti            
        "%u," //mode            
        "%u" //rbid
        ");"
        , tstamp               
        , id->type
        , id->plmn.mcc
        , id->plmn.mnc
        , id->plmn.mnc_digit_len
        , id->nb_id.nb_id 
        , id->cu_du_id ? c_cu_du_id : c_null
        , pdcp->txpdu_pkts     
        , pdcp->txpdu_bytes    
        , pdcp->txpdu_sn       
        , pdcp->rxpdu_pkts     
        , pdcp->rxpdu_bytes    
        , pdcp->rxpdu_sn       
        , pdcp->rxpdu_oo_pkts  
        , pdcp->rxpdu_oo_bytes 
        , pdcp->rxpdu_dd_pkts  
        , pdcp->rxpdu_dd_bytes 
        , pdcp->rxpdu_ro_count 
        , pdcp->txsdu_pkts     
        , pdcp->txsdu_bytes    
        , pdcp->rxsdu_pkts     
        , pdcp->rxsdu_bytes    
        , pdcp->rnti           
        , pdcp->mode           
        , pdcp->rbid           
        );                     
  assert(rc < (int)max && "Not enough space in the char array to write all the data");
  return rc;
}

static
int to_sql_string_ue_slice_rb(global_e2_node_id_t const* id, ue_slice_conf_t const* ues, ue_slice_assoc_t const* u, int64_t tstamp, char* out, size_t out_len)
{
  assert(ues != NULL);
  assert(out != NULL);
  const size_t max = 512;
  assert(out_len >= max);

  char* c_null = NULL;
  char c_cu_du_id[26];
  if (id->cu_du_id) {
    int rc = snprintf(c_cu_du_id, 26, "%lu", *id->cu_du_id);
    assert(rc < (int) max && "Not enough space in the char array to write all the data");
  }

  int rc = 0;
  if (u == NULL) {
    rc = snprintf(out, out_len,
                  "INSERT INTO UE_SLICE VALUES("
                  "%ld,"   // tstamp
                  "%d,"    // ngran_node
                  "%d,"    // mcc
                  "%d,"    // mnc
                  "%d,"    // mnc_digit_len
                  "%d,"    // nb_id
                  "'%s',"  // cu_du_id
                  "%d,"    // dl->len_ue_slices
                  "%d,"    // ues[i]->rnti
                  "%d"     // ues[i]->dl_id
                  ");"
                  , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                  , id->cu_du_id ? c_cu_du_id : c_null
                  , ues->len_ue_slice, -1, -1);
    assert(rc < (int)max && "Not enough space in the char array to write all the data");
    return rc;
  }

  rc = snprintf(out, out_len,
                "INSERT INTO UE_SLICE VALUES("
                "%ld,"   // tstamp
                "%d,"    // ngran_node
                "%d,"    // mcc
                "%d,"    // mnc
                "%d,"    // mnc_digit_len
                "%d,"    // nb_id
                "'%s',"  // cu_du_id
                "%d,"    // dl->len_ue_slices
                "%d,"    // ues[i]->rnti
                "%d"     // ues[i]->dl_id
                ");"
                , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                , id->cu_du_id ? c_cu_du_id : c_null
                , ues->len_ue_slice, u->rnti, u->dl_id);
  assert(rc < (int)max && "Not enough space in the char array to write all the data");
  return rc;
}

static
int to_sql_string_slice_rb(global_e2_node_id_t const* id, ul_dl_slice_conf_t const* slices, fr_slice_t const* s, int64_t tstamp, char* out, size_t out_len)
{
  assert(slices != NULL);
  assert(out != NULL);
  const size_t max = 1024;
  assert(out_len >= max);

  char* c_null = NULL;
  char sched_name[50];
  sched_name[0] = '\0';
  strncat(sched_name, slices->sched_name, slices->len_sched_name);

  char c_cu_du_id[26];
  if (id->cu_du_id) {
    int rc = snprintf(c_cu_du_id, 26, "%lu", *id->cu_du_id);
    assert(rc < (int) max && "Not enough space in the char array to write all the data");
  }

  int rc = 0;
  if (s == NULL) {
    rc = snprintf(out, out_len,
                  "INSERT INTO SLICE VALUES("
                  "%ld,"   // tstamp
                  "%d,"    // ngran_node
                  "%d,"    // mcc
                  "%d,"    // mnc
                  "%d,"    // mnc_digit_len
                  "%d,"    // nb_id
                  "'%s',"  // cu_du_id
                  "%d,"    // dl->len_slices
                  "'%s',"  // dl->sched_name
                  "%u,"    // dl->slice[i].id
                  "'%s',"  // dl->slice[i].label
                  "'%s',"  // dl->slice[i]->params.type
                  "'%s',"  // dl->slice[i]->params.u.nvs.conf
                  "'%s',"  // dl->slice[i].sched
                  "%.2f,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u1.mbps_required/nvs.u.capacity.u.pct_reserved/edf.deadline
                  "%.2f,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u2.mbps_reference/edf.guaranteed_prbs
                  "%.2f"  // dl->slice[i]->params.u.edf.max_replenish
                  ");"
                  , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                  , id->cu_du_id ? c_cu_du_id : c_null
                  , 0, sched_name, 0, c_null, c_null, c_null, c_null, 0.00, 0.00, 0.00);
    assert(rc < (int)max && "Not enough space in the char array to write all the data");
    return rc;
  }

  char label[50];
  label[0] = '\0';
  strncat(label, s->label, s->len_label);
  char params_type[10];
  params_type[0] = '\0';
  char params_type_conf[10];
  params_type_conf[0] = '\0';
  char sched[50];
  sched[0] = '\0';
  strncat(sched, s->sched, s->len_sched);
  if (s->params.type == SLICE_ALG_SM_V0_STATIC) {
    strcat(params_type, "STATIC");
    rc = snprintf(out, out_len,
                  "INSERT INTO SLICE VALUES("
                  "%ld,"   // tstamp
                  "%d,"    // ngran_node
                  "%d,"    // mcc
                  "%d,"    // mnc
                  "%d,"    // mnc_digit_len
                  "%d,"    // nb_id
                  "'%s',"  // cu_du_id
                  "%d,"    // dl->len_slices
                  "'%s',"  // dl->sched_name
                  "%u,"    // dl->slice[i].id
                  "'%s',"  // dl->slice[i].label
                  "'%s',"  // dl->slice[i]->params.type
                  "'%s',"  // dl->slice[i]->params.u.nvs.conf
                  "'%s',"  // dl->slice[i].sched
                  "%d,"    // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u1.mbps_required/nvs.u.capacity.u.pct_reserved/edf.deadline
                  "%d,"    // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u2.mbps_reference/edf.guaranteed_prbs
                  "%.2f"  // dl->slice[i]->params.u.edf.max_replenish
                  ");"
                  , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                  , id->cu_du_id ? c_cu_du_id : c_null
                  , slices->len_slices, c_null
                  , s->id, label, params_type, c_null, sched
                  , s->params.u.sta.pos_low, s->params.u.sta.pos_high, 0.00);
  } else if (s->params.type == SLICE_ALG_SM_V0_NVS) {
    strcat(params_type, "NVS");
    if (s->params.u.nvs.conf == SLICE_SM_NVS_V0_RATE) {
      strcat(params_type_conf, "RATE");
      rc = snprintf(out, out_len,
                    "INSERT INTO SLICE VALUES("
                    "%ld,"   // tstamp
                    "%d,"    // ngran_node
                    "%d,"    // mcc
                    "%d,"    // mnc
                    "%d,"    // mnc_digit_len
                    "%d,"    // nb_id
                    "'%s',"  // cu_du_id
                    "%d,"    // dl->len_slices
                    "'%s',"  // dl->sched_name
                    "%u,"    // dl->slice[i].id
                    "'%s',"  // dl->slice[i].label
                    "'%s',"  // dl->slice[i]->params.type
                    "'%s',"  // dl->slice[i]->params.u.nvs.conf
                    "'%s',"  // dl->slice[i].sched
                    "%.2f,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u1.mbps_required/nvs.u.capacity.u.pct_reserved/edf.deadline
                    "%.2f,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u2.mbps_reference/edf.guaranteed_prbs
                    "%.2f"  // dl->slice[i]->params.u.edf.max_replenish
                    ");"
                    , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                    , id->cu_du_id ? c_cu_du_id : c_null
                    , slices->len_slices, c_null
                    , s->id, label, params_type, params_type_conf, sched
                    , s->params.u.nvs.u.rate.u1.mbps_required, s->params.u.nvs.u.rate.u2.mbps_reference, 0.00);
      } else if (s->params.u.nvs.conf == SLICE_SM_NVS_V0_CAPACITY) {
        strcat(params_type_conf, "CAPACITY");
        rc = snprintf(out, out_len,
                      "INSERT INTO SLICE VALUES("
                      "%ld,"   // tstamp
                      "%d,"    // ngran_node
                      "%d,"    // mcc
                      "%d,"    // mnc
                      "%d,"    // mnc_digit_len
                      "%d,"    // nb_id
                      "'%s',"  // cu_du_id
                      "%d,"    // dl->len_slices
                      "'%s',"  // dl->sched_name
                      "%u,"    // dl->slice[i].id
                      "'%s',"  // dl->slice[i].label
                      "'%s',"  // dl->slice[i]->params.type
                      "'%s',"  // dl->slice[i]->params.u.nvs.conf
                      "'%s',"  // dl->slice[i].sched
                      "%.2f,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u1.mbps_required/nvs.u.capacity.u.pct_reserved/edf.deadline
                      "%.2f,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u2.mbps_reference/edf.guaranteed_prbs
                      "%.2f"  // dl->slice[i]->params.u.edf.max_replenish
                      ");"
                      , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                      , id->cu_du_id ? c_cu_du_id : c_null
                      , slices->len_slices, c_null
                      , s->id, label, params_type, params_type_conf, sched
                      , s->params.u.nvs.u.capacity.u.pct_reserved, 0.00, 0.00);
      }
  } else if (s->params.type == SLICE_ALG_SM_V0_EDF) {
    strcat(params_type, "EDF");
    rc = snprintf(out, out_len,
                  "INSERT INTO SLICE VALUES("
                  "%ld,"   // tstamp
                  "%d,"    // ngran_node
                  "%d,"    // mcc
                  "%d,"    // mnc
                  "%d,"    // mnc_digit_len
                  "%d,"    // nb_id
                  "'%s',"  // cu_du_id
                  "%d,"    // dl->len_slices
                  "'%s',"  // dl->sched_name
                  "%u,"    // dl->slice[i].id
                  "'%s',"  // dl->slice[i].label
                  "'%s',"  // dl->slice[i]->params.type
                  "'%s',"  // dl->slice[i]->params.u.nvs.conf
                  "'%s',"  // dl->slice[i].sched
                  "%d,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u1.mbps_required/nvs.u.capacity.u.pct_reserved/edf.deadline
                  "%d,"  // dl->slice[i]->params.u.sta.pos_high/nvs.u.rate.u2.mbps_reference/edf.guaranteed_prbs
                  "%d"  // dl->slice[i]->params.u.edf.max_replenish
                  ");"
                  , tstamp, id->type, id->plmn.mcc, id->plmn.mnc, id->plmn.mnc_digit_len, id->nb_id.nb_id
                  , id->cu_du_id ? c_cu_du_id : c_null
                  , slices->len_slices, c_null
                  , s->id, label, params_type, c_null, sched
                  , s->params.u.edf.deadline
                  , s->params.u.edf.guaranteed_prbs
                  , s->params.u.edf.max_replenish);
  }
  assert(rc < (int) max && "Not enough space in the char array to write all the data");
  return rc;
}

static
int to_sql_string_gtp_NGUT(global_e2_node_id_t const* id,gtp_ngu_t_stats_t* gtp, int64_t tstamp, char* out, size_t out_len)
{
  assert(gtp != NULL);
  assert(out != NULL);
  const size_t max = 1024;
  assert(out_len >= max);

  char* c_null = NULL;
  char c_cu_du_id[26];
  if (id->cu_du_id) {
    int rc = snprintf(c_cu_du_id, 26, "%lu", *id->cu_du_id);
    assert(rc < (int) max && "Not enough space in the char array to write all the data");
  }

  int const rc = snprintf(out, max,
        "INSERT INTO GTP_NGUT VALUES("
        "%ld," //tstamp         
        "%d," //ngran_node  
        "%d," //mcc
        "%d," //mnc
        "%d," //mnc_digit_len   
        "%d," //nb_id 
        "'%s'," //cu_du_id
        "%u," //teidgnb    
        "%u," //rnti
        "%u," // qfi   
        "%u" //teidupf
        ");"
        , tstamp               
        , id->type
        , id->plmn.mcc
        , id->plmn.mnc
        , id->plmn.mnc_digit_len
        , id->nb_id.nb_id
        , id->cu_du_id ? c_cu_du_id : c_null
        , gtp->teidgnb
        , gtp->rnti   
        , gtp->qfi    
        , gtp->teidupf    
        );      
  assert(rc < (int)max && "Not enough space in the char array to write all the data");
  return rc;
}

// const char* extractLabel(const char* input)
// {
//   const size_t maxLabelLength = 100;
//   static char label[100]; 

//   const char* firstDot = strchr(input, '.');
//   if (firstDot == NULL) {
//       return NULL;
//   }

//   const char* secondDot = strchr(firstDot + 1, '.');
//   if (secondDot == NULL) {
//       return NULL;
//   }

//   size_t labelLength = strlen(secondDot + 1);
//   if (labelLength >= maxLabelLength) {
//       return NULL; 
//   }

//   strcpy(label, secondDot + 1);
//   return label;
// }

static
void to_sql_string_kpm_measRecord(global_e2_node_id_t const* id,
                                  char *name_str,
                                  meas_record_lst_t* kpm_measRecord,
                                  label_info_lst_t const* label_info,
                                  uint64_t tstamp,
                                  char  indication_message_format,
                                  const char* ue_id,
                                  const char* ue_id_group,
                                  const char* ran_ue_id,
                                  char* incomplete_flag_str,
                                  char* out, 
                                  size_t out_len)
{
  assert(id != NULL && out != NULL && out_len >= MAX_SQL_LENGTH);

  char value_buffer[32];
  const char* value_str;
  switch (kpm_measRecord->value) {
    case INTEGER_MEAS_VALUE:
      snprintf(value_buffer, sizeof(value_buffer), "%u", kpm_measRecord->int_val);
      break;
    case REAL_MEAS_VALUE:
      snprintf(value_buffer, sizeof(value_buffer), "%.2f", kpm_measRecord->real_val);
      break;
    case NO_VALUE_MEAS_VALUE:
      strcpy(value_buffer, "NULL");
      break;
    default:
      assert(0 && "Invalid measurement record type");
      return;
  }
  value_str = value_buffer;

  char* ngran_type = get_ngran_name(id->type);

  char bin_x_str[16] = "NULL";
  char bin_y_str[16] = "NULL";
  char bin_z_str[16] = "NULL";

  if (label_info->distBinX) snprintf(bin_x_str, sizeof(bin_x_str), "%u", *label_info->distBinX);
  if (label_info->distBinY) snprintf(bin_y_str, sizeof(bin_y_str), "%u", *label_info->distBinY);
  if (label_info->distBinZ) snprintf(bin_z_str, sizeof(bin_z_str), "%u", *label_info->distBinZ);

  int rc = snprintf(out, out_len,
    "INSERT INTO KPM_MeasRecord VALUES("
    "%lu,'%s',%d,%d,%d,%lu,%u,'%s','%s','%s','%s',%s,%s,%s,'%s','%s');"
    , tstamp
    , ngran_type
    , id->nb_id.nb_id
    , id->plmn.mcc
    , id->plmn.mnc
    , (id->cu_du_id ? *id->cu_du_id : 0)
    , indication_message_format
    , ue_id
    , ue_id_group
    , ran_ue_id
    , name_str
    , bin_x_str
    , bin_y_str
    , bin_z_str
    , value_str
    , incomplete_flag_str
  );
  assert(rc > 0 && rc < (int)out_len && "Not enough space in the char array to write all the data");
}

static
int to_sql_string_rc_meas_report(global_e2_node_id_t const* id,
                                   uint64_t tstamp,
                                   const char* ran_ue_id_str,
                                   long rsrp,
                                   float rsrq,
                                   float sinr,
                                   char* out,
                                   size_t out_len)
{
  assert(id != NULL && out != NULL);
  const size_t max = 512;
  assert(out_len >= max);

  char* ngran_type_str = get_ngran_name(id->type);
  long cu_du_id_val = id->cu_du_id ? *id->cu_du_id : 0;

  int rc = snprintf(out, out_len,
                    "INSERT INTO RC_MEAS_REPORT VALUES("
                    "%lu,'%s',%d,%d,%d,%lu,%s,%ld,%.2f,%.2f);",
                    tstamp,
                    ngran_type_str,
                    id->nb_id.nb_id,
                    id->plmn.mcc,
                    id->plmn.mnc,
                    cu_du_id_val,
                    ran_ue_id_str,
                    rsrp,
                    rsrq,
                    sinr);

  assert(rc > 0 && rc < (int)max);
  return rc;
}

static
void write_mac_stats(sqlite3* db, global_e2_node_id_t const* id, mac_ind_data_t const* ind )
{
  assert(db != NULL);
  assert(ind != NULL);

  mac_ind_msg_t const* ind_msg_mac = &ind->msg; 

  char buffer[2048] = {0};
  int pos = 0;

  for(size_t i = 0; i < ind_msg_mac->len_ue_stats; ++i){
    pos += to_sql_string_mac_ue(id, &ind_msg_mac->ue_stats[i], ind_msg_mac->tstamp, buffer + pos, 2048 - pos);
  }

  insert_db(db, buffer);
}

static
void write_rlc_stats(sqlite3* db, global_e2_node_id_t const* id, rlc_ind_data_t const* ind)
{
  assert(db != NULL);
  assert(ind != NULL);

  rlc_ind_msg_t const* ind_msg_rlc = &ind->msg; 

  char buffer[2048] = {0};
  int pos = 0;

  for(size_t i = 0; i < ind_msg_rlc->len; ++i){
    pos += to_sql_string_rlc_rb(id, &ind_msg_rlc->rb[i], ind_msg_rlc->tstamp, buffer + pos, 2048 - pos);
  }

  insert_db(db, buffer);

}

static
void write_pdcp_stats(sqlite3* db, global_e2_node_id_t const* id, pdcp_ind_data_t const* ind)
{
  assert(db != NULL);
  assert(ind != NULL);

  pdcp_ind_msg_t const* ind_msg_pdcp = &ind->msg; 

  char buffer[2048] = {0};
  int pos = 0;

  for(size_t i = 0; i < ind_msg_pdcp->len; ++i){
    pos += to_sql_string_pdcp_rb(id, &ind_msg_pdcp->rb[i], ind_msg_pdcp->tstamp, buffer + pos, 2048 - pos);
  }

  insert_db(db, buffer);
}

static
void write_slice_conf_stats(sqlite3* db, global_e2_node_id_t const* id, int64_t tstamp, slice_conf_t const* slice_conf)
{
  char buffer[4096] = {0};
  int pos = 0;

  ul_dl_slice_conf_t const* dlslices = &slice_conf->dl;
  if (dlslices->len_slices > 0) {
    for(size_t i = 0; i < dlslices->len_slices; ++i) {
      fr_slice_t const* s = &dlslices->slices[i];
      pos += to_sql_string_slice_rb(id, dlslices, s, tstamp, buffer + pos, 4096 - pos);
    }
  } else {
    pos += to_sql_string_slice_rb(id, dlslices, NULL, tstamp, buffer + pos, 4096 - pos);
  }

  // TODO: Process uplink slice stats

  insert_db(db, buffer);
}

static
void write_ue_slice_conf_stats(sqlite3* db, global_e2_node_id_t const* id, int64_t tstamp, ue_slice_conf_t const* ue_slice_conf)
{
  char buffer[4096] = {0};
  int pos = 0;

  if (ue_slice_conf->len_ue_slice > 0) {
    for(uint32_t j = 0; j < ue_slice_conf->len_ue_slice; ++j) {
      ue_slice_assoc_t *u = &ue_slice_conf->ues[j];
      pos += to_sql_string_ue_slice_rb(id, ue_slice_conf, u, tstamp, buffer + pos, 2048 - pos);
    }
  } else {
    pos += to_sql_string_ue_slice_rb(id, ue_slice_conf, NULL, tstamp, buffer + pos, 2048 - pos);
  }

  insert_db(db, buffer);
}

static
void write_slice_stats(sqlite3* db, global_e2_node_id_t const* id, slice_ind_data_t const* ind)
{
  assert(db != NULL);
  assert(ind != NULL);

  slice_ind_msg_t const* ind_msg_slice = &ind->msg;

  write_slice_conf_stats(db, id, ind_msg_slice->tstamp, &ind_msg_slice->slice_conf);
  write_ue_slice_conf_stats(db, id, ind_msg_slice->tstamp, &ind_msg_slice->ue_slice_conf);

}

static
void write_gtp_stats(sqlite3* db, global_e2_node_id_t const* id, gtp_ind_data_t const* ind)
{
  assert(db != NULL);
  assert(ind != NULL);

  gtp_ind_msg_t const* ind_msg_gtp = &ind->msg; 

  char buffer[2048] = {0};
  int pos = 0;
  for(size_t i = 0; i < ind_msg_gtp->len; ++i){
    pos += to_sql_string_gtp_NGUT(id, &ind_msg_gtp->ngut[i], ind_msg_gtp->tstamp, buffer + pos, 2048 - pos);
  }

  insert_db(db, buffer);
}


static
ue_id_info_t get_gnb_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  if (ue_id.gnb.gnb_cu_ue_f1ap_lst != NULL) {
    for (size_t i = 0; i < ue_id.gnb.gnb_cu_ue_f1ap_lst_len; i++) {
      char temp[32];
      snprintf(temp, sizeof(temp), "%s%u", i > 0 ? ", " : "", ue_id.gnb.gnb_cu_ue_f1ap_lst[i]);
      strcat(result.id, temp);
    }
    strcpy(result.group, "gnb_cu_ue_f1ap");
  } else {
    snprintf(result.id, sizeof(result.id), "%lu", ue_id.gnb.amf_ue_ngap_id);
    strcpy(result.group, "amf_ue_ngap_id");
  }
  
  if (ue_id.gnb.ran_ue_id != NULL) {
    snprintf(result.ran_ue_id, sizeof(result.ran_ue_id), "%lx", *ue_id.gnb.ran_ue_id); // RAN UE NGAP ID
  }
  return result;
}

static
ue_id_info_t get_du_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  snprintf(result.id, sizeof(result.id), "%u", ue_id.gnb_du.gnb_cu_ue_f1ap);
  strcpy(result.group, "gnb_cu_ue_f1ap");

  if (ue_id.gnb_du.ran_ue_id != NULL) {
    snprintf(result.ran_ue_id, sizeof(result.ran_ue_id), "%lx", *ue_id.gnb_du.ran_ue_id); // RAN UE NGAP ID
  }

  return result;
}

static
ue_id_info_t get_cuup_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  snprintf(result.id, sizeof(result.id), "%u", ue_id.gnb_cu_up.gnb_cu_cp_ue_e1ap);
  strcpy(result.group, "gnb_cu_cp_ue_e1ap");

  if (ue_id.gnb_cu_up.ran_ue_id != NULL) {
    snprintf(result.ran_ue_id, sizeof(result.ran_ue_id), "%lx", *ue_id.gnb_cu_up.ran_ue_id); // RAN UE NGAP ID
  }

  return result;
}

static
ue_id_info_t get_ng_enb_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  snprintf(result.id, sizeof(result.id), "%lu", ue_id.ng_enb.amf_ue_ngap_id);
  strcpy(result.group, "amf_ue_ngap_id");

  // no RAN UE ID in LTE RAN
  strcpy(result.ran_ue_id, "NULL");

  return result;
}

static
ue_id_info_t get_ng_enb_du_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  snprintf(result.id, sizeof(result.id), "%u", ue_id.ng_enb_du.ng_enb_cu_ue_w1ap_id);
  strcpy(result.group, "ng_enb_cu_ue_w1ap_id");

  // no RAN UE ID in LTE RAN
  strcpy(result.ran_ue_id, "NULL");

  return result;
}

static
ue_id_info_t get_en_gnb_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  snprintf(result.id, sizeof(result.id), "%u", ue_id.en_gnb.enb_ue_x2ap_id);
  strcpy(result.group, "enb_ue_x2ap_id");

  if (ue_id.en_gnb.ran_ue_id != NULL) {
    snprintf(result.ran_ue_id, sizeof(result.ran_ue_id), "%lx", *ue_id.en_gnb.ran_ue_id); // RAN UE NGAP ID
  }

  return result;
}

static
ue_id_info_t get_enb_ue_id(ue_id_e2sm_t ue_id)
{
  ue_id_info_t result = {0};

  snprintf(result.id, sizeof(result.id), "%u", ue_id.enb.mme_ue_s1ap_id);
  strcpy(result.group, "mme_ue_s1ap_id");

  // no RAN UE ID in LTE
  strcpy(result.ran_ue_id, "NULL");

  return result;
}

typedef ue_id_info_t (*get_ue_id)(ue_id_e2sm_t ue_id);

static
get_ue_id get_ue_id_e2sm[END_UE_ID_E2SM] = {
  get_gnb_ue_id,  // common for gNB-mono, CU and CU-CP
  get_du_ue_id,
  get_cuup_ue_id,
  get_ng_enb_ue_id,
  get_ng_enb_du_ue_id,
  get_en_gnb_ue_id,
  get_enb_ue_id
};

static
char *get_meas_name(meas_type_t meas_type)
{
  char* value = NULL;
  if (meas_type.type == NAME_MEAS_TYPE) {
    value = cp_ba_to_str(meas_type.name);
  } else if (meas_type.type == ID_MEAS_TYPE) {
    value = ecalloc(8, sizeof(char));
    sprintf(value, "%u", meas_type.id);
  }

  return value;
}

static
void process_format_1_message(sqlite3* db, global_e2_node_id_t const* id, kpm_ind_msg_t const* msg, uint64_t collectStartTime)
{
  char buffer[MAX_SQL_LENGTH] = {0};
  kpm_ind_msg_format_1_t const* frm_1 = &msg->frm_1;
  for(size_t i = 0; i < frm_1->meas_data_lst_len; i++){
    meas_data_lst_t* meas_data_item = &frm_1->meas_data_lst[i];
    char* incomplete_flag_str = (meas_data_item->incomplete_flag != NULL && *meas_data_item->incomplete_flag == TRUE_ENUM_VALUE) ? "TRUE" : "FALSE";

    for (size_t z = 0; z < frm_1->meas_info_lst_len; z++) {
      const meas_info_format_1_lst_t* meas_info_item = &frm_1->meas_info_lst[z];
      char *name_str = get_meas_name(meas_info_item->meas_type);
      for (size_t j = 0; j < meas_info_item->label_info_lst_len; j++) {
        meas_record_lst_t* meas_record_item = &meas_data_item->meas_record_lst[z + j];
        to_sql_string_kpm_measRecord(id, name_str, meas_record_item, &meas_info_item->label_info_lst[j],
                                     collectStartTime, 1, NULL, NULL, NULL, incomplete_flag_str, 
                                     buffer, MAX_SQL_LENGTH);
        insert_db(db, buffer);
      }
      free(name_str);
    }
  }
}

static
void process_format_2_message(sqlite3* db, global_e2_node_id_t const* id, kpm_ind_msg_t const* msg, uint64_t collectStartTime)
{
  (void)db;
  (void)id;
  (void)msg;
  (void)collectStartTime;
  printf("Processing IND MSG Format 2 for exporting in the database not tested.\n");
/*
  char buffer[MAX_SQL_LENGTH] = {0};
  kpm_ind_msg_format_2_t const* frm_2 = &msg->frm_2;
  for(size_t i = 0; i < frm_2->meas_data_lst_len; i++){
    meas_data_lst_t* meas_data_item = &frm_2->meas_data_lst[i];
    char* incomplete_flag_str = (meas_data_item->incomplete_flag != NULL && *meas_data_item->incomplete_flag == TRUE_ENUM_VALUE) ? "TRUE" : "FALSE";

    for (size_t z = 0; z < frm_2->meas_info_cond_ue_lst_len; z++) {
      const meas_info_cond_ue_lst_t* info_cond_ue_item = &frm_2->meas_info_cond_ue_lst[z];
      const meas_type_t meas_type = info_cond_ue_item->meas_type;
      char *name_str = cp_ba_to_str(meas_type.name);
      for (size_t j = 0; j < info_cond_ue_item->matching_cond_lst_len; j++){
	const matching_condition_format_3_lst_t* cond_frm_3 = &info_cond_ue_item->matching_cond_lst[j];
        meas_record_lst_t* meas_record_item = &meas_data_item->meas_record_lst[z + j];
        to_sql_string_kpm_measRecord(id, name_str, meas_record_item, &cond_frm_3->label_info_lst,
                                      collectStartTime, 2, NULL, NULL, NULL, incomplete_flag_str,
                                      buffer, MAX_SQL_LENGTH);
        insert_db(db, buffer);
      }
      free(name_str);
    }
  }
*/
}

static
void process_format_3_message(sqlite3* db, global_e2_node_id_t const* id, kpm_ind_msg_t const* msg, uint64_t collectStartTime)
{
  char buffer[MAX_SQL_LENGTH] = {0};
  kpm_ind_msg_format_3_t const* frm_3 = &msg->frm_3;
  for(size_t i = 0; i < frm_3->ue_meas_report_lst_len; i++){
    meas_report_per_ue_t* ue_report_item = &frm_3->meas_report_per_ue[i];
    kpm_ind_msg_format_1_t* frm_1 = &ue_report_item->ind_msg_format_1;

    // Get UE ID
    ue_id_e2sm_t const ue_id_e2sm = frm_3->meas_report_per_ue[i].ue_meas_report_lst;
    ue_id_e2sm_e const type = ue_id_e2sm.type;
    ue_id_info_t ue_id_info = get_ue_id_e2sm[type](ue_id_e2sm);
    
    for(size_t j = 0; j < frm_1->meas_data_lst_len; j++){
      meas_data_lst_t* meas_data_item = &frm_1->meas_data_lst[j];
      char* incomplete_flag_str = (meas_data_item->incomplete_flag != NULL && *meas_data_item->incomplete_flag == TRUE_ENUM_VALUE) ? "TRUE" : "FALSE";
      
      for (size_t z = 0; z < frm_1->meas_info_lst_len; z++) {
        const meas_info_format_1_lst_t* meas_info_item = &frm_1->meas_info_lst[z];
        char *name_str = get_meas_name(meas_info_item->meas_type);
        for (size_t i = 0; i < meas_info_item->label_info_lst_len; i++){
          meas_record_lst_t* meas_record_item = &meas_data_item->meas_record_lst[z + i];
          to_sql_string_kpm_measRecord(id, name_str, meas_record_item, &meas_info_item->label_info_lst[i],
                                        collectStartTime, 3, ue_id_info.id, ue_id_info.group, ue_id_info.ran_ue_id, incomplete_flag_str, 
                                        buffer, MAX_SQL_LENGTH);
          insert_db(db, buffer);
        }
        free(name_str);
      }
    }
  }
}

typedef void (*process_message_func)(sqlite3* db, global_e2_node_id_t const* id, kpm_ind_msg_t const* msg, uint64_t collectStartTime);

// Array of function pointers for message processing
static process_message_func process_message[END_INDICATION_MESSAGE] = {
  process_format_1_message,
  process_format_2_message,
  process_format_3_message,
};

static
void write_kpm_stats(sqlite3* db, global_e2_node_id_t const* id, kpm_ind_data_t const* ind)
{
  assert(db != NULL);
  assert(ind != NULL);

  uint64_t collectStartTime;

  // Check the header format and get collectStartTime
  switch(ind->hdr.type) {
    case FORMAT_1_INDICATION_HEADER:
      collectStartTime = time_now_us()/1000000;
      break;
    case END_INDICATION_HEADER:
      fprintf(stderr, "Indication header format not defined\n");
      return;
    default:
      fprintf(stderr, "Unknown indication header format\n");
      return;
  }

  // Process the indication message
  if (ind->msg.type < END_INDICATION_MESSAGE) {
    process_message[ind->msg.type](db, id, &ind->msg, collectStartTime);
  } else {
    fprintf(stderr, "Unexpected KPM indication message format\n");
  }
}

static
void write_rc_stats(sqlite3* db, global_e2_node_id_t const* id, rc_ind_data_t const* ind)
{
  assert(db != NULL && id != NULL && ind != NULL);

  char buffer[MAX_SQL_LENGTH * 4] = {0};
  int pos = 0;

  if (ind->msg.format != FORMAT_1_E2SM_RC_IND_MSG) return;
  const e2sm_rc_ind_msg_frmt_1_t* msg = &ind->msg.frmt_1;
  byte_array_t rrc_msg_ba = {0};
  ue_id_e2sm_t ue_id = {0};

  for (size_t i = 0; i < msg->sz_seq_ran_param; ++i) {
    seq_ran_param_t* param = &msg->seq_ran_param[i];
    if (param->ran_param_id == E2SM_RC_RS1_RRC_MESSAGE) {
      rrc_msg_ba = param->ran_param_val.flag_false->octet_str_ran;
    } else if (param->ran_param_id == E2SM_RC_RS1_UE_ID) {
      // at the moment, UE ID not used
      /*
      UEID_t ue_id_asn = {0};
      UEID_t* src_ref = &ue_id_asn;

      byte_array_t *ba = &param->ran_param_val.flag_false->octet_str_ran;
      asn_dec_rval_t const ret = aper_decode(NULL, &asn_DEF_UEID, (void **)&src_ref, ba->buf, ba->len, 0, 0);
      assert(ret.code == RC_OK);

      ue_id = dec_ue_id_asn(&ue_id_asn);
      ASN_STRUCT_RESET(asn_DEF_UEID, &ue_id_asn);
      */
    }
  }

  if (rrc_msg_ba.buf == NULL) return;

  NR_UL_DCCH_Message_t *ul_msg = NULL;
  uper_decode(NULL, &asn_DEF_NR_UL_DCCH_Message, (void **)&ul_msg, rrc_msg_ba.buf, rrc_msg_ba.len, 0, 0);

  if (!ul_msg || ul_msg->message.present != NR_UL_DCCH_MessageType_PR_c1 || ul_msg->message.choice.c1->present != NR_UL_DCCH_MessageType__c1_PR_measurementReport) {
    ASN_STRUCT_FREE(asn_DEF_NR_UL_DCCH_Message, ul_msg);
    return;
  }

  NR_MeasResults_t *results = &ul_msg->message.choice.c1->choice.measurementReport->criticalExtensions.choice.measurementReport->measResults;
  uint64_t tstamp = time_now_us() / 1000000;
  char ran_ue_id_str[32] = "NULL";
  if (ue_id.gnb.ran_ue_id) snprintf(ran_ue_id_str, sizeof(ran_ue_id_str), "'%lx'", *ue_id.gnb.ran_ue_id);

    // Process Serving Cell Measurements - Mirroring log_meas_report()
  for (int i = 0; i < results->measResultServingMOList.list.count; i++) {
    NR_MeasResultServMO_t *serv_mo = results->measResultServingMOList.list.array[i];
    NR_MeasResultNR_t *meas_nr = &serv_mo->measResultServingCell;
    if (meas_nr->measResult.cellResults.resultsSSB_Cell) {
      NR_MeasQuantityResults_t *mqr = meas_nr->measResult.cellResults.resultsSSB_Cell;
      long rsrp = mqr->rsrp ? (*mqr->rsrp - 156) : -300;
      float rsrq = mqr->rsrq ? ((*mqr->rsrq - 87) / 2.0f) : -300;
      float sinr = mqr->sinr ? ((*mqr->sinr - 46) / 2.0f) : -300;
      pos += to_sql_string_rc_meas_report(id, tstamp, ran_ue_id_str, rsrp, rsrq, sinr, buffer + pos, sizeof(buffer) - pos);
    }
  }

  if (pos > 0) {
    insert_db(db, buffer);
  }

  ASN_STRUCT_FREE(asn_DEF_NR_UL_DCCH_Message, ul_msg);
  free_ue_id_e2sm(&ue_id);
}

void init_db_sqlite3(sqlite3** db, char const* db_filename)
{
  assert(db != NULL);
  assert(db_filename != NULL);

  int const rc = sqlite3_open(db_filename, db);
  assert(rc != SQLITE_CANTOPEN && "SQLITE3 cannot open the directory. Does it already exist?");
  assert(rc == SQLITE_OK && "Error while creating the DB at /tmp/db_xapp");

  // Optimizations. Write Ahead Logging
  char* err_msg = NULL;
  int const rc_2 = sqlite3_exec(*db, "pragma journal_mode=wal" , 0, 0, &err_msg);
  assert(rc_2 == SQLITE_OK && "Error while setting the wal mode in sqlite3");

  int const rc_3 = sqlite3_exec(*db, "pragma synchronous=normal" , 0, 0, &err_msg);
  assert(rc_3 == SQLITE_OK && "Error while setting the syncronous mode to normal");


  //////
  // MAC
  //////
  create_mac_ue_table(*db);

  //////
  // RLC
  //////
  create_rlc_bearer_table(*db);

  //////
  // PDCP
  //////
  create_pdcp_bearer_table(*db);

  //////
  // SLICE
  //////
  create_slice_table(*db);
  create_ue_slice_table(*db);

  ////
  // GTP
  ////
  create_gtp_table(*db);

  ////
  // KPM
  ////
  create_kpm_table(*db);

  ////
  // RC Measurement Report
  ////
  create_rc_meas_report_table(*db);
}

void close_db_sqlite3(sqlite3* db)
{
  assert(db != NULL);
  int const rc = sqlite3_close(db);
  assert(rc == SQLITE_OK && "Error while closing the DB");
}

void write_db_sqlite3(sqlite3* db, global_e2_node_id_t const* id, sm_ag_if_rd_t const* ag_rd)
{
  assert(db != NULL);
  assert(ag_rd != NULL);
  assert(ag_rd->type == INDICATION_MSG_AGENT_IF_ANS_V0);

  sm_ag_if_rd_ind_t const* rd = &ag_rd->ind; 
  assert(rd->type == MAC_STATS_V0   || rd->type == RLC_STATS_V0 
      || rd->type == PDCP_STATS_V0  || rd->type == SLICE_STATS_V0 
      || rd->type == KPM_STATS_V3_0 || rd->type == GTP_STATS_V0
      || rd->type == RAN_CTRL_STATS_V1_03);

  if(rd->type == MAC_STATS_V0){
    write_mac_stats(db, id, &rd->mac);
  } else if(rd->type == RLC_STATS_V0 ){
    write_rlc_stats(db, id, &rd->rlc);
  } else if( rd->type == PDCP_STATS_V0) {
    write_pdcp_stats(db, id, &rd->pdcp);
  } else if (rd->type == SLICE_STATS_V0) {
    write_slice_stats(db, id, &rd->slice);
  } else if (rd->type == GTP_STATS_V0) {
    write_gtp_stats(db, id, &rd->gtp);
  } else if (rd->type == KPM_STATS_V3_0) {
    write_kpm_stats(db, id, &ag_rd->ind.kpm.ind);
  } else if(rd->type ==  RAN_CTRL_STATS_V1_03){
    write_rc_stats(db, id, &rd->rc.ind);
  } else {
    assert(0!=0 && "Unknown statistics type received ");
  }
}


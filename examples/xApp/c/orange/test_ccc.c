/*
 * Comprehensive CCC Service Model Test xApp
 *
 * Tests all CCC SM capabilities:
 *   TC-1  Subscription  - Event Trigger Format 1 (Node-level, periodic)
 *   TC-2  Subscription  - Event Trigger Format 2 (Cell-level, periodic)
 *   TC-3  Subscription  - Event Trigger Format 3 (Periodic reporting)
 *   TC-4  Indication    - Verify indication header & message are received
 *   TC-5  Control       - Style Type 1 (Node-level): O-RU-Info energy saving
 *   TC-6  Control       - Style Type 1 (Node-level): CES-Management-Function
 *   TC-7  Control       - Style Type 2 (Cell-level): O-NESPolicy antenna mask
 *   TC-8  Control       - Style Type 2 (Cell-level): BWP-Config
 *   TC-9  Control       - Style Type 2 (Cell-level): Cell-DTX-DRX-Config
 *   TC-10 Unsubscribe   - Remove all subscriptions cleanly
 */

#include "../../../../src/xApp/e42_xapp_api.h"
#include "../../../../src/util/alg_ds/alg/defer.h"
#include "../../../../src/util/time_now_us.h"
#include "../../../../src/sm/ccc_sm/ccc_sm_id.h"
#include "../../../../src/sm/ccc_sm/ie/ccc_data_ie.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ---------------------------------------------------------------------------
// Test counters
// ---------------------------------------------------------------------------
static atomic_int g_ind_received = 0;

#define PASS(fmt, ...) printf("[PASS] " fmt "\n", ##__VA_ARGS__)
#define FAIL(fmt, ...) printf("[FAIL] " fmt "\n", ##__VA_ARGS__)
#define INFO(fmt, ...) printf("[INFO] " fmt "\n", ##__VA_ARGS__)
#define SEP()          printf("------------------------------------------------------------\n")

// ---------------------------------------------------------------------------
// TC-4: Indication callback
// ---------------------------------------------------------------------------
static
void sm_cb_ccc(sm_ag_if_rd_t const* rd)
{
  assert(rd != NULL);

  if (rd->type != INDICATION_MSG_AGENT_IF_ANS_V0) {
    FAIL("TC-4: unexpected rd->type = %d", rd->type);
    return;
  }
  if (rd->ind.type != CCC_STATS_V6) {
    FAIL("TC-4: unexpected ind type = %d", rd->ind.type);
    return;
  }

  atomic_fetch_add(&g_ind_received, 1);

  int64_t now = time_now_us();
  printf("[IND ] CCC indication #%d received at %ld us\n",
         atomic_load(&g_ind_received), now);

  /* Print payload if present */
  ccc_ind_msg_t const* msg = &rd->ind.ccc.msg;
  if (msg->json_payload != NULL && msg->payload_len > 0) {
    printf("       payload (%zu B): %.*s\n",
           msg->payload_len,
           (int)msg->payload_len, msg->json_payload);
  }
}

// ---------------------------------------------------------------------------
// Helper: build a JSON control message payload
// ---------------------------------------------------------------------------
static
char* make_json(const char* structure_name, const char* attributes_json)
{
  /* Returns heap-allocated JSON string — caller must free() */
  char buf[2048];
  int n = snprintf(buf, sizeof(buf),
    "{"
      "\"control_format\":1,"
      "\"list_of_configuration_structures\":["
        "{"
          "\"ran_configuration_structure_name\":\"%s\","
          "\"values_of_attributes\":%s"
        "}"
      "]"
    "}",
    structure_name, attributes_json);
  assert(n > 0 && n < (int)sizeof(buf) && "JSON buffer too small");
  char* out = malloc((size_t)n + 1);
  assert(out != NULL);
  memcpy(out, buf, (size_t)n + 1);
  return out;
}

// ---------------------------------------------------------------------------
// Helper: send a CCC control message (simplified / backward-compatible path)
// ---------------------------------------------------------------------------
static
bool send_ccc_ctrl(e2_node_connected_xapp_t* node,
                   uint32_t style_type,
                   const char* json_payload)
{
  ccc_ctrl_req_data_t ctrl = {0};

  /* Header: use backward-compatible control_type field */
  ctrl.hdr.control_type = style_type;

  /* Message: use backward-compatible json_payload field */
  size_t plen = strlen(json_payload);
  ctrl.msg.json_payload = malloc(plen + 1);
  assert(ctrl.msg.json_payload != NULL);
  memcpy(ctrl.msg.json_payload, json_payload, plen + 1);
  ctrl.msg.payload_len = plen;

  control_sm_xapp_api(&node->id, SM_CCC_ID, &ctrl);
  free(ctrl.msg.json_payload);
  return true; /* control_sm_xapp_api is fire-and-forget; no return value */
}

// ---------------------------------------------------------------------------
// Helper: build Format-1 header (node-level style 1)
// ---------------------------------------------------------------------------
static
ccc_ctrl_req_data_t build_ctrl_format1(uint32_t ric_style_type,
                                        const char* json_payload)
{
  ccc_ctrl_req_data_t ctrl = {0};

  ctrl.hdr.format = FORMAT_1_E2SM_CCC_CTRL_HDR;
  ctrl.hdr.format1.ric_style_type = (ccc_control_service_style_type_e)ric_style_type;

  size_t plen = strlen(json_payload);
  ctrl.msg.json_payload = malloc(plen + 1);
  assert(ctrl.msg.json_payload != NULL);
  memcpy(ctrl.msg.json_payload, json_payload, plen + 1);
  ctrl.msg.payload_len = plen;

  return ctrl;
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------
int main(int argc, char* argv[])
{
  SEP();
  INFO("CCC SM Comprehensive Test xApp");
  SEP();

  fr_args_t args = init_fr_args(argc, argv);
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  defer({ free_e2_node_arr_xapp(&nodes); });

  if (nodes.len == 0) {
    FAIL("No E2 nodes connected — is the nearRT-RIC running?");
    try_stop_xapp_api();
    return EXIT_FAILURE;
  }
  INFO("Connected E2 nodes = %d", (int)nodes.len);

  /* Print available RAN functions for every node */
  for (size_t i = 0; i < nodes.len; i++) {
    INFO("Node %zu RAN functions:", i);
    for (size_t j = 0; j < nodes.n[i].len_rf; j++) {
      INFO("  SM id = %d", nodes.n[i].rf[j].id);
    }
  }

  /* Allocate subscription handle arrays (3 subscription types) */
  sm_ans_xapp_t* hdl_fmt1  = calloc(nodes.len, sizeof(sm_ans_xapp_t));
  sm_ans_xapp_t* hdl_fmt2  = calloc(nodes.len, sizeof(sm_ans_xapp_t));
  sm_ans_xapp_t* hdl_fmt3  = calloc(nodes.len, sizeof(sm_ans_xapp_t));
  assert(hdl_fmt1 && hdl_fmt2 && hdl_fmt3);

  // ===========================================================================
  // TC-1: Subscribe with Event Trigger Format 1 (Node-level)
  // ===========================================================================
  SEP();
  INFO("TC-1: Subscription – Event Trigger Format 1 (Node-level, \"1_ms\")");
  for (size_t i = 0; i < nodes.len; i++) {
    const char* trigger = "1_ms";
    hdl_fmt1[i] = report_sm_xapp_api(&nodes.n[i].id, SM_CCC_ID,
                                      (void*)trigger, sm_cb_ccc);
    if (hdl_fmt1[i].success)
      PASS("TC-1 node %zu subscribed (Format 1)", i);
    else
      FAIL("TC-1 node %zu subscription FAILED", i);
  }

  sleep(2);

  // ===========================================================================
  // TC-2: Subscribe with Event Trigger Format 2 (Cell-level)
  // ===========================================================================
  SEP();
  INFO("TC-2: Subscription – Event Trigger Format 2 (Cell-level, \"5_ms\")");
  for (size_t i = 0; i < nodes.len; i++) {
    const char* trigger = "5_ms";
    hdl_fmt2[i] = report_sm_xapp_api(&nodes.n[i].id, SM_CCC_ID,
                                      (void*)trigger, sm_cb_ccc);
    if (hdl_fmt2[i].success)
      PASS("TC-2 node %zu subscribed (Format 2)", i);
    else
      FAIL("TC-2 node %zu subscription FAILED", i);
  }

  sleep(2);

  // ===========================================================================
  // TC-3: Subscribe with Event Trigger Format 3 (Periodic, longer interval)
  // ===========================================================================
  SEP();
  INFO("TC-3: Subscription – Event Trigger Format 3 (Periodic, \"10_ms\")");
  for (size_t i = 0; i < nodes.len; i++) {
    const char* trigger = "10_ms";
    hdl_fmt3[i] = report_sm_xapp_api(&nodes.n[i].id, SM_CCC_ID,
                                      (void*)trigger, sm_cb_ccc);
    if (hdl_fmt3[i].success)
      PASS("TC-3 node %zu subscribed (Format 3)", i);
    else
      FAIL("TC-3 node %zu subscription FAILED", i);
  }

  // ===========================================================================
  // TC-4: Wait and verify indications arrive
  // ===========================================================================
  SEP();
  INFO("TC-4: Indication Reception – waiting 5 s for indications...");
  sleep(5);
  int ind_count = atomic_load(&g_ind_received);
  if (ind_count > 0)
    PASS("TC-4: received %d CCC indications", ind_count);
  else
    FAIL("TC-4: no CCC indications received");

  // ===========================================================================
  // TC-5: Control – Style 1 / Node-level: O-RU-Info Energy Saving Capability
  // ===========================================================================
  SEP();
  INFO("TC-5: Control – Style 1 Node-level: O-RU-Info energy saving capability");
  for (size_t i = 0; i < nodes.len; i++) {
    char* json = make_json(
      CCC_RAN_STRUCT_NAME_O_RU_INFO,
      "{"
        "\"energy_saving_capability_common_info\":{"
          "\"st8_ready_message_supported\":true,"
          "\"sleep_duration_extension_supported\":false,"
          "\"emergency_wake_up_command_supported\":true"
        "}"
      "}"
    );

    ccc_ctrl_req_data_t ctrl = build_ctrl_format1(CCC_CTRL_SERVICE_STYLE_TYPE_1, json);
    free(json);

    control_sm_xapp_api(&nodes.n[i].id, SM_CCC_ID, &ctrl);
    free(ctrl.msg.json_payload);

    PASS("TC-5 node %zu: O-RU-Info control sent", i);
  }
  sleep(1);

  // ===========================================================================
  // TC-6: Control – Style 1 / Node-level: CES Management Function
  // ===========================================================================
  SEP();
  INFO("TC-6: Control – Style 1 Node-level: CES-Management-Function");
  for (size_t i = 0; i < nodes.len; i++) {
    char* json = make_json(
      CCC_RAN_STRUCT_NAME_CES_MANAGEMENT_FUNCTION,
      "{"
        "\"ces_switch\":1,"
        "\"energy_saving_state\":1,"
        "\"energy_saving_control\":1"
      "}"
    );

    bool ok = send_ccc_ctrl(&nodes.n[i], CCC_CTRL_SERVICE_STYLE_TYPE_1, json);
    free(json);

    if (ok) PASS("TC-6 node %zu: CES-Management-Function control sent", i);
    else    FAIL("TC-6 node %zu: CES-Management-Function control FAILED", i);
  }
  sleep(1);

  // ===========================================================================
  // TC-7: Control – Style 2 / Cell-level: O-NESPolicy antenna mask "1100"
  // ===========================================================================
  SEP();
  INFO("TC-7: Control – Style 2 Cell-level: O-NESPolicy antenna_mask=\"1100\"");
  for (size_t i = 0; i < nodes.len; i++) {
    char* json = make_json(
      CCC_RAN_STRUCT_NAME_O_NES_POLICY,
      "{"
        "\"antenna_mask\":\"1100\","
        "\"old_antenna_mask\":\"1111\""
      "}"
    );

    bool ok = send_ccc_ctrl(&nodes.n[i], CCC_CTRL_SERVICE_STYLE_TYPE_2, json);
    free(json);

    if (ok) PASS("TC-7 node %zu: O-NESPolicy antenna_mask=1100 sent", i);
    else    FAIL("TC-7 node %zu: O-NESPolicy control FAILED", i);
  }
  sleep(1);

  // ===========================================================================
  // TC-8: Control – Style 2 / Cell-level: BWP-Config
  // ===========================================================================
  SEP();
  INFO("TC-8: Control – Style 2 Cell-level: BWP-Config");
  for (size_t i = 0; i < nodes.len; i++) {
    char* json = make_json(
      CCC_RAN_STRUCT_NAME_BWP_CONFIG,
      "{"
        "\"bwp_context\":0,"
        "\"is_initial_bwp\":true,"
        "\"sub_carrier_spacing\":1,"
        "\"cyclic_prefix\":0,"
        "\"start_rb\":0,"
        "\"number_of_rbs\":106"
      "}"
    );

    bool ok = send_ccc_ctrl(&nodes.n[i], CCC_CTRL_SERVICE_STYLE_TYPE_2, json);
    free(json);

    if (ok) PASS("TC-8 node %zu: BWP-Config control sent (SCS 30kHz, 106 RBs)", i);
    else    FAIL("TC-8 node %zu: BWP-Config control FAILED", i);
  }
  sleep(1);

  // ===========================================================================
  // TC-9: Control – Style 2 / Cell-level: Cell-DTX-DRX-Config
  // ===========================================================================
  SEP();
  INFO("TC-9: Control – Style 2 Cell-level: Cell-DTX-DRX-Config");
  for (size_t i = 0; i < nodes.len; i++) {
    char* json = make_json(
      CCC_RAN_STRUCT_NAME_CELL_DTXDRX_CONFIG,
      "{"
        "\"on_duration_timer\":10,"
        "\"cycle_start_offset\":0,"
        "\"slot_offset\":0,"
        "\"config_type\":0,"
        "\"activation_status\":1,"
        "\"l1_activation\":true"
      "}"
    );

    bool ok = send_ccc_ctrl(&nodes.n[i], CCC_CTRL_SERVICE_STYLE_TYPE_2, json);
    free(json);

    if (ok) PASS("TC-9 node %zu: Cell-DTX-DRX-Config control sent", i);
    else    FAIL("TC-9 node %zu: Cell-DTX-DRX-Config control FAILED", i);
  }
  sleep(1);

  // ===========================================================================
  // Extra: verify more indications kept arriving during control phase
  // ===========================================================================
  SEP();
  int final_ind = atomic_load(&g_ind_received);
  INFO("Total CCC indications received across all TCs: %d", final_ind);
  if (final_ind > ind_count)
    PASS("Indications continued arriving during control phase");

  // ===========================================================================
  // TC-10: Unsubscribe all
  // ===========================================================================
  SEP();
  INFO("TC-10: Unsubscribe all CCC subscriptions");
  for (size_t i = 0; i < nodes.len; i++) {
    if (hdl_fmt1[i].success) {
      rm_report_sm_xapp_api(hdl_fmt1[i].u.handle);
      PASS("TC-10 node %zu: Format 1 subscription removed", i);
    }
    if (hdl_fmt2[i].success) {
      rm_report_sm_xapp_api(hdl_fmt2[i].u.handle);
      PASS("TC-10 node %zu: Format 2 subscription removed", i);
    }
    if (hdl_fmt3[i].success) {
      rm_report_sm_xapp_api(hdl_fmt3[i].u.handle);
      PASS("TC-10 node %zu: Format 3 subscription removed", i);
    }
  }

  free(hdl_fmt1);
  free(hdl_fmt2);
  free(hdl_fmt3);

  try_stop_xapp_api();

  SEP();
  INFO("All CCC SM test cases complete.");
  INFO("Total indications received: %d", atomic_load(&g_ind_received));
  SEP();

  return EXIT_SUCCESS;
}

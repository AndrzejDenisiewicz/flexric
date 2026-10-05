/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * The monitor implementation contains the KPM model-version glue used by
 * this FlexRIC checkout.  Reusing it keeps subscriptions comparable with the
 * reference monitor while the decision and transport boundary remain local
 * to this xApp.
 */
#define main rf_reconfiguration_monitor_reference_main
#include "../monitor/xapp_kpm_moni.c"
#undef main

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef struct {
  double power_threshold;
  double hysteresis;
  uint64_t cooldown_ms;
  bool dry_run;
  const char* transport;
  int node_index;
} rf_config_t;

typedef struct {
  double last_value;
  uint64_t last_decision_ms;
  bool active;
} rf_cell_state_t;

static rf_config_t config;
static rf_cell_state_t cell_state = {0};

static uint64_t
monotonic_ms(void)
{
  struct timespec ts = {0};
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return (uint64_t)ts.tv_sec * 1000U + (uint64_t)ts.tv_nsec / 1000000U;
}

static double
env_double(const char* name, double fallback)
{
  const char* value = getenv(name);
  if (value == NULL || *value == '\0') {
    return fallback;
  }
  char* end = NULL;
  errno = 0;
  double result = strtod(value, &end);
  return errno == 0 && end != value && *end == '\0' ? result : fallback;
}

static uint64_t
env_uint64(const char* name, uint64_t fallback)
{
  const char* value = getenv(name);
  if (value == NULL || *value == '\0') {
    return fallback;
  }
  char* end = NULL;
  errno = 0;
  unsigned long long result = strtoull(value, &end, 10);
  return errno == 0 && end != value && *end == '\0' ? (uint64_t)result : fallback;
}

static bool
env_bool(const char* name, bool fallback)
{
  const char* value = getenv(name);
  if (value == NULL) {
    return fallback;
  }
  return strcmp(value, "1") == 0 || strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0;
}

static void
load_config(void)
{
  config.power_threshold = env_double("RF_XAPP_POWER_THRESHOLD", 0.0);
  config.hysteresis = env_double("RF_XAPP_HYSTERESIS", 1.0);
  config.cooldown_ms = env_uint64("RF_XAPP_COOLDOWN_MS", 5000);
  config.dry_run = env_bool("RF_XAPP_DRY_RUN", true);
  config.transport = getenv("RF_XAPP_TRANSPORT");
  if (config.transport == NULL || *config.transport == '\0') {
    config.transport = "kpm-first";
  }
  const char* node = getenv("RF_XAPP_NODE_INDEX");
  config.node_index = node == NULL ? -1 : atoi(node);

  printf("[rf_reconfiguration_xapp] transport=%s threshold=%.3f hysteresis=%.3f "
         "cooldown_ms=%lu dry_run=%s node_index=%d\n",
         config.transport,
         config.power_threshold,
         config.hysteresis,
         (unsigned long)config.cooldown_ms,
         config.dry_run ? "true" : "false",
         config.node_index);
}

static void
emit_intent(double value)
{
  uint64_t now = monotonic_ms();
  bool above = value >= config.power_threshold + config.hysteresis;
  bool below = value <= config.power_threshold - config.hysteresis;
  if (!above && !below) {
    return;
  }
  if (cell_state.active == above && now - cell_state.last_decision_ms < config.cooldown_ms) {
    return;
  }

  cell_state.active = above;
  cell_state.last_value = value;
  cell_state.last_decision_ms = now;
  printf("[rf_reconfiguration_xapp] intent=antenna-state state=%s value=%.3f "
         "transport=%s dry_run=%s\n",
         above ? "active" : "idle",
         value,
         config.transport,
         config.dry_run ? "true" : "false");

  if (config.dry_run || strcmp(config.transport, "kpm-first") == 0) {
    return;
  }
  if (strcmp(config.transport, "native-ccc") == 0) {
    printf("[rf_reconfiguration_xapp] diagnostic=native-ccc-unavailable "
           "reason=FlexRIC checkout has no CCC encoder/API\n");
  } else if (strcmp(config.transport, "rc-compatibility") == 0) {
    printf("[rf_reconfiguration_xapp] diagnostic=rc-adapter-not-configured "
           "reason=typed intent retained without CCC claim\n");
  } else {
    printf("[rf_reconfiguration_xapp] diagnostic=unknown-transport value=%s\n", config.transport);
  }
}

static void
rf_kpm_callback(sm_ag_if_rd_t const* rd)
{
  if (rd == NULL || rd->type != INDICATION_MSG_AGENT_IF_ANS_V0 || rd->ind.type != KPM_STATS_V3_0) {
    printf("[rf_reconfiguration_xapp] diagnostic=unsupported-kpm-indication\n");
    return;
  }

  kpm_ind_data_t const* ind = &rd->ind.kpm.ind;
  pthread_mutex_lock(&mtx);
  if (ind->msg.type == FORMAT_1_INDICATION_MESSAGE) {
    for (size_t i = 0; i < ind->msg.frm_1.meas_data_lst_len; ++i) {
      meas_data_lst_t const* data = &ind->msg.frm_1.meas_data_lst[i];
      for (size_t j = 0; j < data->meas_record_len; ++j) {
        if (data->meas_record_lst[j].value == REAL_MEAS_VALUE) {
          emit_intent(data->meas_record_lst[j].real_val);
          break;
        }
      }
    }
  } else if (ind->msg.type == FORMAT_3_INDICATION_MESSAGE) {
    for (size_t i = 0; i < ind->msg.frm_3.ue_meas_report_lst_len; ++i) {
      kpm_ind_msg_format_1_t const* report =
          &ind->msg.frm_3.meas_report_per_ue[i].ind_msg_format_1;
      for (size_t j = 0; j < report->meas_data_lst_len; ++j) {
        meas_data_lst_t const* data = &report->meas_data_lst[j];
        for (size_t k = 0; k < data->meas_record_len; ++k) {
          if (data->meas_record_lst[k].value == REAL_MEAS_VALUE) {
            emit_intent(data->meas_record_lst[k].real_val);
            break;
          }
        }
      }
    }
  }
  pthread_mutex_unlock(&mtx);
}

int
main(int argc, char* argv[])
{
  fr_args_t args = init_fr_args(argc, argv);
  load_config();
  init_xapp_api(&args);
  sleep(1);

  e2_node_arr_xapp_t nodes = e2_nodes_xapp_api();
  assert(nodes.len > 0 && "No E2 nodes connected");
  printf("[rf_reconfiguration_xapp] connected_nodes=%d\n", nodes.len);

  pthread_mutexattr_t attr = {0};
  assert(pthread_mutex_init(&mtx, &attr) == 0);
  const int kpm_ran_function = 2;
  sm_ans_xapp_t** handles = calloc(nodes.len, sizeof(*handles));
  assert(handles != NULL);

  for (size_t i = 0; i < nodes.len; ++i) {
    if (config.node_index >= 0 && (int)i != config.node_index) {
      continue;
    }
    e2_node_connected_xapp_t* node = &nodes.n[i];
    size_t idx = find_sm_idx(node->rf, node->len_rf, eq_sm, kpm_ran_function);
    if (node->rf[idx].defn.type != KPM_RAN_FUNC_DEF_E) {
      printf("[rf_reconfiguration_xapp] diagnostic=kpm-function-type-mismatch node=%zu\n", i);
      continue;
    }
    size_t styles = node->rf[idx].defn.kpm.sz_ric_report_style_list;
    handles[i] = calloc(styles, sizeof(**handles));
    assert(handles[i] != NULL);
    for (size_t j = 0; j < styles; ++j) {
      kpm_sub_data_t subscription = gen_kpm_subs(&node->rf[idx].defn.kpm,
                                                 &node->rf[idx].defn.kpm.ric_report_style_list[j]);
      handles[i][j] = report_sm_xapp_api(&node->id, kpm_ran_function, &subscription, rf_kpm_callback);
      if (!handles[i][j].success) {
        printf("[rf_reconfiguration_xapp] diagnostic=kpm-subscription-failed node=%zu style=%zu\n", i, j);
      }
      free_kpm_sub_data(&subscription);
    }
  }

  xapp_wait_end_api();
  for (size_t i = 0; i < nodes.len; ++i) {
    if (handles[i] == NULL) {
      continue;
    }
    e2_node_connected_xapp_t* node = &nodes.n[i];
    size_t idx = find_sm_idx(node->rf, node->len_rf, eq_sm, kpm_ran_function);
    for (size_t j = 0; j < node->rf[idx].defn.kpm.sz_ric_report_style_list; ++j) {
      if (handles[i][j].success) {
        rm_report_sm_xapp_api(handles[i][j].u.handle);
      }
    }
    free(handles[i]);
  }
  free(handles);
  free_e2_node_arr_xapp(&nodes);
  while (!try_stop_xapp_api()) {
    usleep(1000);
  }
  pthread_mutex_destroy(&mtx);
  return 0;
}
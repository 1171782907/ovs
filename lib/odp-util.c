/*
 * Copyright (c) 2009, 2010 Nicira Networks.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <config.h>
#include "odp-util.h"
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "coverage.h"
#include "dynamic-string.h"
#include "flow.h"
#include "packets.h"
#include "timeval.h"
#include "util.h"

union odp_action *
odp_actions_add(struct odp_actions *actions, uint16_t type)
{
    union odp_action *a;
    if (actions->n_actions < MAX_ODP_ACTIONS) {
        a = &actions->actions[actions->n_actions++];
    } else {
        COVERAGE_INC(odp_overflow);
        actions->n_actions = MAX_ODP_ACTIONS + 1;
        a = &actions->actions[MAX_ODP_ACTIONS - 1];
    }
    memset(a, 0, sizeof *a);
    a->type = type;
    return a;
}

void
format_odp_flow_key(struct ds *ds, const struct odp_flow_key *key)
{
    ds_put_format(ds, "in_port%04x:vlan%d:pcp%d mac"ETH_ADDR_FMT
                  "->"ETH_ADDR_FMT" type%04x proto%"PRId8" tos%"PRIu8
                  " ip"IP_FMT"->"IP_FMT" port%d->%d",
                  key->in_port, ntohs(key->dl_vlan), key->dl_vlan_pcp,
                  ETH_ADDR_ARGS(key->dl_src), ETH_ADDR_ARGS(key->dl_dst),
                  ntohs(key->dl_type), key->nw_proto, key->nw_tos,
                  IP_ARGS(&key->nw_src), IP_ARGS(&key->nw_dst),
                  ntohs(key->tp_src), ntohs(key->tp_dst));
}

void
format_odp_action(struct ds *ds, const union odp_action *a)
{
    switch (a->type) {
    case ODPAT_OUTPUT:
        ds_put_format(ds, "%"PRIu16, a->output.port);
        break;
    case ODPAT_OUTPUT_GROUP:
        ds_put_format(ds, "g%"PRIu16, a->output_group.group);
        break;
    case ODPAT_CONTROLLER:
        ds_put_format(ds, "ctl(%"PRIu32")", a->controller.arg);
        break;
    case ODPAT_SET_VLAN_VID:
        ds_put_format(ds, "set_vlan(%"PRIu16")", ntohs(a->vlan_vid.vlan_vid));
        break;
    case ODPAT_SET_VLAN_PCP:
        ds_put_format(ds, "set_vlan_pcp(%"PRIu8")", a->vlan_pcp.vlan_pcp);
        break;
    case ODPAT_STRIP_VLAN:
        ds_put_format(ds, "strip_vlan");
        break;
    case ODPAT_SET_DL_SRC:
        ds_put_format(ds, "set_dl_src("ETH_ADDR_FMT")",
               ETH_ADDR_ARGS(a->dl_addr.dl_addr));
        break;
    case ODPAT_SET_DL_DST:
        ds_put_format(ds, "set_dl_dst("ETH_ADDR_FMT")",
               ETH_ADDR_ARGS(a->dl_addr.dl_addr));
        break;
    case ODPAT_SET_NW_SRC:
        ds_put_format(ds, "set_nw_src("IP_FMT")",
                      IP_ARGS(&a->nw_addr.nw_addr));
        break;
    case ODPAT_SET_NW_DST:
        ds_put_format(ds, "set_nw_dst("IP_FMT")",
                      IP_ARGS(&a->nw_addr.nw_addr));
        break;
    case ODPAT_SET_NW_TOS:
        ds_put_format(ds, "set_nw_tos(%"PRIu8")", a->nw_tos.nw_tos);
        break;
    case ODPAT_SET_TP_SRC:
        ds_put_format(ds, "set_tp_src(%"PRIu16")", ntohs(a->tp_port.tp_port));
        break;
    case ODPAT_SET_TP_DST:
        ds_put_format(ds, "set_tp_dst(%"PRIu16")", ntohs(a->tp_port.tp_port));
        break;
    default:
        ds_put_format(ds, "***bad action %"PRIu16"***", a->type);
        break;
    }
}

void
format_odp_actions(struct ds *ds, const union odp_action *actions,
                   size_t n_actions)
{
    size_t i;
    for (i = 0; i < n_actions; i++) {
        if (i) {
            ds_put_char(ds, ',');
        }
        format_odp_action(ds, &actions[i]);
    }
    if (!n_actions) {
        ds_put_cstr(ds, "drop");
    }
}

void
format_odp_flow_stats(struct ds *ds, const struct odp_flow_stats *s)
{
    ds_put_format(ds, "packets:%llu, bytes:%llu, used:",
                  (unsigned long long int) s->n_packets,
                  (unsigned long long int) s->n_bytes);
    if (s->used_sec) {
        long long int used = s->used_sec * 1000 + s->used_nsec / 1000000;
        ds_put_format(ds, "%.3fs", (time_msec() - used) / 1000.0);
    } else {
        ds_put_format(ds, "never");
    }
}

void
format_odp_flow(struct ds *ds, const struct odp_flow *f)
{
    format_odp_flow_key(ds, &f->key);
    ds_put_cstr(ds, ", ");
    format_odp_flow_stats(ds, &f->stats);
    ds_put_cstr(ds, ", actions:");
    format_odp_actions(ds, f->actions, f->n_actions);
}

void
odp_flow_key_from_flow(struct odp_flow_key *key, const struct flow *flow)
{
    key->nw_src = flow->nw_src;
    key->nw_dst = flow->nw_dst;
    key->in_port = flow->in_port;
    key->dl_vlan = flow->dl_vlan;
    key->dl_type = flow->dl_type;
    key->tp_src = flow->tp_src;
    key->tp_dst = flow->tp_dst;
    memcpy(key->dl_src, flow->dl_src, ETH_ALEN);
    memcpy(key->dl_dst, flow->dl_dst, ETH_ALEN);
    key->nw_proto = flow->nw_proto;
    key->dl_vlan_pcp = flow->dl_vlan_pcp;
    key->nw_tos = flow->nw_tos;
    memset(key->reserved, 0, sizeof key->reserved);
}

void
odp_flow_key_to_flow(const struct odp_flow_key *key, struct flow *flow)
{
    flow->nw_src = key->nw_src;
    flow->nw_dst = key->nw_dst;
    flow->in_port = key->in_port;
    flow->dl_vlan = key->dl_vlan;
    flow->dl_type = key->dl_type;
    flow->tp_src = key->tp_src;
    flow->tp_dst = key->tp_dst;
    memcpy(flow->dl_src, key->dl_src, ETH_ALEN);
    memcpy(flow->dl_dst, key->dl_dst, ETH_ALEN);
    flow->nw_proto = key->nw_proto;
    flow->dl_vlan_pcp = key->dl_vlan_pcp;
    flow->nw_tos = key->nw_tos;
}

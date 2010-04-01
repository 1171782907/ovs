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

#ifndef ODP_UTIL_H
#define ODP_UTIL_H 1

#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "hash.h"
#include "openflow/openflow.h"
#include "openvswitch/datapath-protocol.h"

struct ds;
struct flow;

/* The kernel datapaths limits actions to those that fit in a single page of
 * memory, so there is no point in allocating more than that.  */
enum { MAX_ODP_ACTIONS = 4096 / sizeof(union odp_action) };

struct odp_actions {
    size_t n_actions;
    union odp_action actions[MAX_ODP_ACTIONS];
};

static inline void
odp_actions_init(struct odp_actions *actions)
{
    actions->n_actions = 0;
}

union odp_action *odp_actions_add(struct odp_actions *actions, uint16_t type);

static inline bool
odp_actions_overflow(const struct odp_actions *actions)
{
    return actions->n_actions > MAX_ODP_ACTIONS;
}

static inline uint16_t
ofp_port_to_odp_port(uint16_t ofp_port)
{
    switch (ofp_port) {
    case OFPP_LOCAL:
        return ODPP_LOCAL;
    case OFPP_NONE:
        return ODPP_NONE;
    default:
        return ofp_port;
    }
}

static inline uint16_t
odp_port_to_ofp_port(uint16_t odp_port)
{
    switch (odp_port) {
    case ODPP_LOCAL:
        return OFPP_LOCAL;
    case ODPP_NONE:
        return OFPP_NONE;
    default:
        return odp_port;
    }
}

void format_odp_flow_key(struct ds *, const struct odp_flow_key *);
void format_odp_action(struct ds *, const union odp_action *);
void format_odp_actions(struct ds *, const union odp_action *actions,
                        size_t n_actions);
void format_odp_flow_stats(struct ds *, const struct odp_flow_stats *);
void format_odp_flow(struct ds *, const struct odp_flow *);

void odp_flow_key_from_flow(struct odp_flow_key *, const struct flow *);
void odp_flow_key_to_flow(const struct odp_flow_key *, struct flow *);

static inline bool
odp_flow_key_equal(const struct odp_flow_key *a, const struct odp_flow_key *b)
{
    return !memcmp(a, b, sizeof *a);
}

static inline size_t
odp_flow_key_hash(const struct odp_flow_key *flow, uint32_t basis)
{
    BUILD_ASSERT_DECL(!(sizeof *flow % sizeof(uint32_t)));
    return hash_words((const uint32_t *) flow,
                      sizeof *flow / sizeof(uint32_t), basis);
}

#endif /* odp-util.h */

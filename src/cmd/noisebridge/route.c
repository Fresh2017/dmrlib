#include <string.h>
#include <stdlib.h>
#include <dmr.h>
#include <dmr/error.h>
#include <dmr/log.h>
#include "config.h"
#include "route.h"
#include "util.h"

static const char *route_rule_syntax = "name:{policy,proto,proto,ts,pi,flco,dmr_id,dmr_id}";

int parsebool(char *v, bool *b)
{
    if (!strcmp(v, "yes") || !strcmp(v, "1") || !strcmp(v, "on")) {
        *b = true;
        return 0;
    }
    if (!strcmp(v, "no") || !strcmp(v, "0") || !strcmp(v, "off")) {
        *b = false;
        return 0;
    }
    return 1;
}

void route_rule_free(route_rule_t *rule)
{
    if (rule == NULL)
        return;

    if (rule->name != NULL)
        free(rule->name);
    if (rule->dst_proto != NULL)
        free(rule->dst_proto);
    if (rule->src_proto != NULL)
        free(rule->src_proto);

    free(rule);
}

route_rule_t *route_rule_parse(char *line)
{
    if (line == NULL)
        return NULL;

    route_rule_t *rule = malloc(sizeof(route_rule_t));
    if (rule == NULL) {
        dmr_error(DMR_ENOMEM);
        return NULL;
    }
    memset(rule, 0, sizeof(route_rule_t));

    char *name, *rest, *part;
    name = strtok_r(line, ":", &rest);
    if (name == NULL || rest == NULL)
        goto bail_route_rule_parse;

    trim(name);
    trim(rest);

    if (strlen(rest) < 2 || rest[0] != '{' || rest[strlen(rest) - 1] != '}')
        goto bail_route_rule_parse;

    rule->name = strdup(name);

    part = strtok_r(rest + 1, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_critical("noisebridge: missing polcy part");
        goto bail_route_rule_parse;
    }
    if (!strcmp("permit", part)) {
        rule->policy = ROUTE_PERMIT;
    } else if (!strcmp("reject", part)) {
        rule->policy = ROUTE_REJECT;
    } else {
        dmr_log_critical("noisebridge: invalid policy \"%s\"", part);
        goto bail_route_rule_parse;
    }

    part = strtok_r(rest + 1, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_critical("noisebridge: missing src proto part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule src proto: %s", part);
    if (strncmp(part, "*", 1)) {
        rule->src_proto = strdup(part);
    }

    part = strtok_r(rest, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_critical("noisebridge: missing dst proto part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule dst proto: %s", part);
    if (strncmp(part, "*", 1)) {
        rule->dst_proto = strdup(part);
    }


    part = strtok_r(rest, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_critical("noisebridge: missing modify part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route modify: %s", part);
    if (parsebool(part, &rule->modify) != 0) {
        dmr_log_critical("noisebridge: expected boolean value");
        goto bail_route_rule_parse;
    }
    if (!rule->modify) {
        dmr_log_debug("noisebridge: stop evaluating route, not modifying");
        goto done_route_rule_parse;
    }

    part = strtok_r(rest, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_critical("noisebridge: missing timeslot part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule timeslot: %s", part);
    rule->ts = atoi(part);
    if (rule->ts == 0) {
        rule->ts = DMR_TS_INVALID; /* do not rewrite ts */
    } else if (rule->ts >= 1 || rule->ts <= 2) {
        rule->ts--;
    } else {
        dmr_log_critical("noisebridge: invalid timeslot %d", rule->ts);
        goto bail_route_rule_parse;
    }

    part = strtok_r(rest, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_trace("noisebridge: missing flco part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule flco: %s", part);
    rule->flco = atoi(part);
    if (rule->flco == 0) {
        rule->flco = DMR_FLCO_INVALID; /* do not rewrite flco */
    } else if (rule->flco == 1 || rule->flco ==  2) {
        rule->flco--;
    } else {
        dmr_log_critical("noisebridge: invalid flco %d", rule->flco);
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule flco parsed to: %s",
        dmr_flco_name(rule->flco));

    part = strtok_r(rest, ",", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_trace("noisebridge: missing src id part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule src id: %s", part);
    rule->src_id = atoi(part);

    part = strtok_r(rest, "}", &rest);
    trim(part);
    if (part == NULL || strlen(part) == 0) {
        dmr_log_trace("noisebridge: missing dst id part");
        goto bail_route_rule_parse;
    }
    dmr_log_trace("noisebridge: route rule dst id: %s", part);
    rule->dst_id = atoi(part);

    goto done_route_rule_parse;

bail_route_rule_parse:
    dmr_log_critical("noisebridge: expected %s", route_rule_syntax);
    route_rule_free(rule);
    return NULL;

done_route_rule_parse:
    if (rule != NULL) {
        dmr_log_debug("noisebridge: route[%s]: proto=%s->%s, ts=%s, flco=%s, id=%u->%u",
            rule->name,
            rule->src_proto, rule->dst_proto,
            dmr_ts_name(rule->ts),
            dmr_flco_name(rule->flco),
            rule->src_id, rule->dst_id);
    }

    return rule;
}

bool route_rule_packet(dmr_repeater_t *repeater, dmr_proto_t *src_proto, dmr_proto_t *dst_proto, dmr_packet_t *packet)
{
    if (repeater == NULL || src_proto == NULL || dst_proto == NULL || packet == NULL)
        return false;

    config_t *config = load_config();
    dmr_log_debug("noisebridge: route: type=%s, proto=%s->%s, ts=%s, flco=%s, id=%u->%u",
        dmr_data_type_name(packet->data_type),
        src_proto->name, dst_proto->name,
        dmr_ts_name(packet->ts),
        dmr_flco_name(packet->flco),
        packet->src_id, packet->dst_id);

    size_t i = 0;
    for (; i < config->repeater_routes; i++) {
        route_rule_t *rule = config->repeater_route[i];
        if ((rule->src_proto == NULL || !strcmp(rule->src_proto, src_proto->name)) &&
            (rule->dst_proto == NULL || !strcmp(rule->dst_proto, dst_proto->name))) {

            //rule->flco = DMR_FLCO_PRIVATE;

            dmr_log_trace("noisebridge: evaluating route policy %s", rule->name);
            if (rule->modify) {
                if (rule->ts != DMR_TS_INVALID) {
                    dmr_log_debug("noisebridge: route[%s]: modify ts %u -> %u", rule->name, packet->ts + 1, rule->ts + 1);
                    packet->ts = rule->ts;
                } else {
                    dmr_log_trace("noisebridge: route[%s]: retain ts %u", rule->name, packet->ts + 1);
                }
                if (rule->src_id) {
                    dmr_log_debug("noisebridge: route[%s]: modify src_id %lu -> %lu", rule->name, packet->src_id, rule->src_id);
                    packet->src_id = rule->src_id;
                } else {
                    dmr_log_trace("noisebridge: route[%s]: retain src_id %lu", rule->name, packet->src_id);
                }
                if (rule->dst_id) {
                    dmr_log_debug("noisebridge: route[%s]: modify dst_id %lu -> %lu", rule->name, packet->dst_id, rule->dst_id);
                    packet->dst_id = rule->dst_id;
                } else {
                    dmr_log_trace("noisebridge: route[%s]: retain dst_id %lu", rule->name, packet->dst_id);
                }
                if (rule->flco != DMR_FLCO_INVALID) {
                    dmr_log_debug("noisebridge: route[%s]: modify flco %u -> %u", rule->name, packet->flco, rule->flco);
                    packet->flco = rule->flco;
                } else {
                    dmr_log_trace("noisebridge: route[%s]: retain flco %u", rule->name, packet->flco);
                }
            } else {
                dmr_log_debug("noisebridge: route[%s]: do not modify", rule->name);
            }
            if (rule->policy == ROUTE_REJECT) {
                dmr_log_debug("noisebridge: route[%s]: rejected by policy", rule->name);
                return false;
            }
            dmr_log_debug("noisebridge: route[%s]: permitted by policy", rule->name);
        }
    }

    return true;
}

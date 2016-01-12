#include <string.h>
#include <stdlib.h>
#include <dmr.h>
#include <dmr/error.h>
#include <dmr/log.h>
#include "config.h"
#include "route.h"
#include "util.h"

static const char *route_rule_syntax = "name:{proto,proto,ts,dmr_id,dmr_id}";

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
    rule = NULL;

done_route_rule_parse:
    if (rule != NULL) {
        dmr_log_debug("noisebridge: route rule \"%s\": [%s->%s], ts %d, [%d->%d]",
            rule->name, rule->src_proto, rule->dst_proto, rule->ts + 1,
            rule->src_id, rule->dst_id);
    }

    return rule;
}

bool route_rule_packet(dmr_repeater_t *repeater, dmr_proto_t *src_proto, dmr_proto_t *dst_proto, dmr_packet_t *packet)
{
    if (repeater == NULL || src_proto == NULL || dst_proto == NULL || packet == NULL)
        return false;

    config_t *config = load_config();
    dmr_log_debug("noisebridge: route %s from proto %s->%s for %d->%d on ts %d",
        dmr_slot_type_name(packet->slot_type),
        src_proto->name, dst_proto->name,
        packet->src_id, packet->dst_id,
        packet->ts + 1);

    size_t i = 0;
    for (; i < config->repeater_routes; i++) {
        route_rule_t *rule = config->repeater_route[i];
        if ((rule->src_proto == NULL || !strcmp(rule->src_proto, src_proto->name)) &&
            (rule->dst_proto == NULL || !strcmp(rule->dst_proto, dst_proto->name))) {

            if (rule->ts != DMR_TS_INVALID)
                packet->ts = rule->ts;
            if (rule->src_id)
                packet->src_id = rule->src_id;
            if (rule->dst_id)
                packet->dst_id = rule->dst_id;
            if (rule->policy == ROUTE_REJECT) {
                dmr_log_debug("noisebridge: route rejected by policy");
                return false;
            }
        }
    }

    return true;
}

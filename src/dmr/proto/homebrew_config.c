#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dmr/log.h"
#include "dmr/proto/homebrew.h"

static const char *dmr_software_id = "dmrlib-20160112";
static const char *dmr_package_id = "git:dmrlib-20160112";
static const char *dmr_location = "Unknown";
static const char *dmr_description = "dmrlib by PD0MZ";
static const char *dmr_url = "https://github.com/pd0mz";

#define CONFIG_VAR(buf, fmt, arg) do {                              \
    char tmp[sizeof(buf) + 1];                                      \
    snprintf(tmp, sizeof(tmp), fmt, arg);                           \
    memcpy(buf, tmp, sizeof(buf));                                  \
    dmr_log_debug("homebrew: config " #arg " = " #fmt, arg); \
} while (0)

dmr_homebrew_config_t *dmr_homebrew_config_new(void)
{
    dmr_homebrew_config_t *config = malloc(sizeof(dmr_homebrew_config_t));
    if (config == NULL)
        return NULL;

    memset(config, 0, sizeof(dmr_homebrew_config_t));
    dmr_homebrew_config_repeater_id(config, 0);
    dmr_homebrew_config_tx_power(config, 0);
    dmr_homebrew_config_rx_freq(config, 0);
    dmr_homebrew_config_tx_freq(config, 0);
    dmr_homebrew_config_latitude(config, 0.0);
    dmr_homebrew_config_longitude(config, 0.0);
    dmr_homebrew_config_color_code(config, 1);
    dmr_homebrew_config_height(config, 0);
    dmr_homebrew_config_location(config, dmr_location);
    dmr_homebrew_config_description(config, dmr_description);
    dmr_homebrew_config_url(config, dmr_url);
    dmr_homebrew_config_software_id(config, dmr_software_id);
    dmr_homebrew_config_package_id(config, dmr_package_id);

    return config;
}

void dmr_homebrew_config_callsign(dmr_homebrew_config_t *config, const char *callsign)
{
    CONFIG_VAR(config->callsign, "%-8s", callsign);
}

void dmr_homebrew_config_repeater_id(dmr_homebrew_config_t *config, dmr_id_t repeater_id)
{
    /*
    config->repeater_id[0] = repeater_id >> 24;
    config->repeater_id[1] = repeater_id >> 16;
    config->repeater_id[2] = repeater_id >> 8;
    config->repeater_id[3] = repeater_id;
    */
    CONFIG_VAR(config->repeater_id, "%08x", repeater_id);
}

void dmr_homebrew_config_rx_freq(dmr_homebrew_config_t *config, uint32_t rx_freq)
{
    CONFIG_VAR(config->rx_freq, "%09u", rx_freq);
}

void dmr_homebrew_config_tx_freq(dmr_homebrew_config_t *config, uint32_t tx_freq)
{
    CONFIG_VAR(config->tx_freq, "%09u", tx_freq);
}

void dmr_homebrew_config_latitude(dmr_homebrew_config_t *config, float latitude)
{
    CONFIG_VAR(config->latitude, "%-08f", latitude);
}

void dmr_homebrew_config_longitude(dmr_homebrew_config_t *config, float longitude)
{
    CONFIG_VAR(config->longitude, "%-09f", longitude);
}

void dmr_homebrew_config_tx_power(dmr_homebrew_config_t *config, uint8_t tx_power)
{
    CONFIG_VAR(config->tx_power, "%02u", tx_power);
}

void dmr_homebrew_config_color_code(dmr_homebrew_config_t *config, dmr_color_code_t color_code)
{
    if (color_code < 1)
        color_code = 1;
    if (color_code > 15)
        color_code = 15;

    CONFIG_VAR(config->color_code, "%02u", color_code);
}

void dmr_homebrew_config_height(dmr_homebrew_config_t *config, int height)
{
    if (height < -99)
        height = -99;
    if (height > 999)
        height = 999;

    CONFIG_VAR(config->height, "%03d", height);
}

void dmr_homebrew_config_location(dmr_homebrew_config_t *config, const char *location)
{
    if (location == NULL)
        return;
    CONFIG_VAR(config->location, "%-20.20s", location);
}

void dmr_homebrew_config_description(dmr_homebrew_config_t *config, const char *description)
{
    if (description == NULL)
        return;
    CONFIG_VAR(config->description, "%-20.20s", description);
}

void dmr_homebrew_config_url(dmr_homebrew_config_t *config, const char *url)
{
    if (url == NULL)
        return;
    CONFIG_VAR(config->url, "%-124.124s", url);
}

void dmr_homebrew_config_software_id(dmr_homebrew_config_t *config, const char *software_id)
{
    if (software_id == NULL)
        return;
    CONFIG_VAR(config->software_id, "%-40.40s", software_id);
}

void dmr_homebrew_config_package_id(dmr_homebrew_config_t *config, const char *package_id)
{
    if (package_id == NULL)
        return;
    CONFIG_VAR(config->package_id, "%-40.40s", package_id);
}

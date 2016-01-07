#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dmr/proto/homebrew.h"

static const char *dmr_software_id = "dmrlib-20160105";
static const char *dmr_package_id = "git:dmrlib-20160105";
static const char *dmr_latitude = "+00.0000";
static const char *dmr_longitude = "+000.0000";
static const char *dmr_location = "Unknown";
static const char *dmr_description = "dmrlib by PD0MZ";
static const char *dmr_url = "https://github.com/pd0mz";

dmr_homebrew_config_t *dmr_homebrew_config_new(void)
{
    dmr_homebrew_config_t *config = malloc(sizeof(dmr_homebrew_config_t));
    if (config == NULL)
        return NULL;

    memset(config, 0, sizeof(dmr_homebrew_config_t));
    memset(config->rx_freq, '0', sizeof(config->rx_freq));
    memset(config->tx_freq, '0', sizeof(config->tx_freq));
    memcpy(config->latitude, dmr_latitude, 8);
    memcpy(config->longitude, dmr_longitude, 9);
    dmr_homebrew_config_repeater_id(config, 0);
    dmr_homebrew_config_tx_power(config, 0);
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
    fprintf(stderr, "homebrew config: call sign = %s\n", callsign);
    int len = min(sizeof(config->callsign), strlen(callsign));
    memset(&config->callsign, 0x20, sizeof(config->callsign));
    memcpy(&config->callsign, callsign, len);
}

void dmr_homebrew_config_repeater_id(dmr_homebrew_config_t *config, dmr_id_t repeater_id)
{
    if (repeater_id != 0) {
        fprintf(stderr, "homebrew config: repeater id = %u, %u\n", repeater_id, repeater_id >> 16);
    }
    config->repeater_id[0] = repeater_id >> 24;
    config->repeater_id[1] = repeater_id >> 16;
    config->repeater_id[2] = repeater_id >> 8;
    config->repeater_id[3] = repeater_id;
}

void dmr_homebrew_config_tx_power(dmr_homebrew_config_t *config, uint8_t tx_power)
{
    if (tx_power > 99)
        tx_power = 99;

    config->tx_power[0] = (tx_power / 10) + '0';
    config->tx_power[1] = (tx_power % 10) + '0';
}

void dmr_homebrew_config_color_code(dmr_homebrew_config_t *config, dmr_color_code_t color_code)
{
    if (color_code < 1)
        color_code = 1;
    if (color_code > 15)
        color_code = 15;

    config->color_code[0] = (color_code / 10) + '0';
    config->color_code[1] = (color_code % 10) + '0';
}

void dmr_homebrew_config_height(dmr_homebrew_config_t *config, int height)
{
    if (height < 0)
        height = 0;
    if (height > 999)
        height = 999;

    int h = (height / 100) % 10,
        t = (height /  10) % 10,
        d = (height %  10);

    config->height[0] = h + '0';
    config->height[1] = t + '0';
    config->height[2] = d + '0';
}

void dmr_homebrew_config_location(dmr_homebrew_config_t *config, const char *location)
{
    int len = min(sizeof(config->location), strlen(location));
    memset(&config->location, 0x20, sizeof(config->location));
    memcpy(&config->location, location, len);
}

void dmr_homebrew_config_description(dmr_homebrew_config_t *config, const char *description)
{
    int len = min(20, strlen(description));
    memset(&config->description, 0x20, 20);
    memcpy(&config->description, description, len);
}

void dmr_homebrew_config_url(dmr_homebrew_config_t *config, const char *url)
{
    int len = min(124, strlen(url));
    memset(&config->url, 0x20, 124);
    memcpy(&config->url, url, len);
}

void dmr_homebrew_config_software_id(dmr_homebrew_config_t *config, const char *software_id)
{
    int len = min(40, strlen(software_id));
    memset(&config->software_id, 0x20, 40);
    memcpy(&config->software_id, software_id, len);
}

void dmr_homebrew_config_package_id(dmr_homebrew_config_t *config, const char *package_id)
{
    int len = min(40, strlen(package_id));
    memset(&config->package_id, 0x20, 40);
    memcpy(&config->package_id, package_id, len);
}

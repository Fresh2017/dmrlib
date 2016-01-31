#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <talloc.h>
#include "common/serial.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    serial_t **ports = talloc_zero(NULL, serial_t *);
    size_t i;
    if (serial_list(&ports) != 0) {
        fprintf(stderr, "port list failed: %s\n", strerror(errno));
        return 1;
    }

    serial_t *port = NULL;
    for (i = 0; ports[i] != NULL; i++) {
        port = ports[i];
        printf("port: %-24s ", serial_name(port));
        switch (serial_transport(port)) {
        case SERIAL_TRANSPORT_NATIVE:
            puts("native");
            break;
        case SERIAL_TRANSPORT_USB: {
            int bus = 0, addr = 0, vid = 0, pid = 0;
            serial_usb_bus_address(port, &bus, &addr);
            serial_usb_vid_pid(port, &vid, &pid);
            printf("USB       bus %03d device %03d: id=%04X:%04X\n%-41sserial=%s\n%-41sproduct=%s\n%-41smanufacturer=%s\n",
                bus, addr, vid, pid, " ",
                serial_usb_serial(ports[i]), " ",
                serial_usb_product(ports[i]), " ",
                serial_usb_manufacturer(ports[i]));
            break;
        }
        case SERIAL_TRANSPORT_BLUETOOTH: {
            char *addr = NULL;
            if ((addr = serial_bluetooth_address(port))) {
                printf("Bluetooth %s\n", addr);
            } else {
                printf("Bluetooth 00:00:00:00:00:00\n");
            }
            break;
        }
        default:
            puts("invalid");
            break;
        }
    }

    return 0;
}

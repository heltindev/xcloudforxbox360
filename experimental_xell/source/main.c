/* XeLL-only Xbox 360 PowerPC boot proof; does not stream games. */
#include <stdio.h>
#include <stdlib.h>
#include <xenos/xenos.h>
#include <console/console.h>
#include <input/input.h>
#include <usb/usbmain.h>

int main(void) {
    struct controller_data_s pad;
    xenos_init(VIDEO_MODE_AUTO);
    console_init();
    usb_init();
    usb_do_poll();
    console_clrscr();

    printf("XCloud360 / XeLL PowerPC boot proof\n");
    printf("Status: boot OK; network not initialized.\n");
    printf("This does not access xCloud or log into Microsoft.\n");
    printf("Press Y or Guide to exit.\n");

    for (;;) {
        usb_do_poll();
        if (get_controller_data(&pad, 0)) {
            if (pad.y || pad.logo) {
                printf("Exiting test.\n");
                exit(0);
            }
        }
    }
    return 0;
}

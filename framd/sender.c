#include "util.h"

/*
 * Sends a bit 1 by repeatedly flushing the address and sends a bit 0 by doing nothing.
 */
void send_bit(bool one, struct config *config)
{
    CYCLES start_t = cc_sync();
    if (one) {
        ADDR_PTR addr = config->addr;
        while ((get_time() - start_t) < config->interval) {
            clflush(addr);
        }
    } else {
        while (get_time() - start_t < config->interval) {}
    }
}

int main(int argc, char **argv) {
    struct config config;
    init_config(&config, argc, argv);
    int sending = 1;

    bool sequence[8] = {1, 0, 1, 0, 1, 0, 1, 1};

    printf("Please type a message (exit to stop).\n");
    while (sending) {
        printf("< ");
        char text_buf[128];
        fgets(text_buf, sizeof(text_buf), stdin);

        if (strcmp(text_buf, "exit\n") == 0) {
            sending = 0;
        }

        char *msg = string_to_binary(text_buf);

        for (int i = 0; i < 8; i++) {
            send_bit(sequence[i], &config);
        }

        size_t msg_len = strlen(msg);
        for (int ind = 0; ind < msg_len; ind++) {
            if (msg[ind] == '0') {
                send_bit(false, &config);
            } else {
                send_bit(true, &config);
            }
        }

        free(msg);
    }

    printf("Sender finished\n");
    return 0;
}

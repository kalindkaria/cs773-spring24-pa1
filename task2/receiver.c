#include <stdio.h>
#include "utils.h"

int main() {

    // Update these values accordingly
    char received_msg[MAX_MSG_SIZE];
    int received_msg_size = 0;

    // declare handle for file mapping
    map_handle_t *handle;

    // map file in memory
    map_file("msg.txt", &handle);

    // receive file contents
    uint32_t sequenceMask = ((uint32_t) 1 << 6) - 1;
    uint32_t expSequence = 0b101011;
    uint32_t bitSequence = 0;
    int i = 0;
    bool break_val = false;

    while (1) {

        bool bitReceived = detect_bit(handle);
        bitSequence = ((uint32_t) bitSequence << 1) | bitReceived;
        if ((bitSequence & sequenceMask) == expSequence) {
            // to keep count of all 0s
            int strike_zeros = 0;

            // detection algorithm - loop over max_length till a sequence of 8 zeros seen
            for (i = 0; i < MAX_MSG_SIZE/8; i++) {
                if ((i & 7) == 7) received_msg_size++;
                bool det = detect_bit(handle);
                if (det) {
                    received_msg[i] = '1';
                    strike_zeros = 0;
                } else {
                    received_msg[i] = '0';
                    if (++strike_zeros >= 16 && i % 8 == 0) {
                        break_val = true;
                        break;
                    }
                }
            }

            // print out message
            received_msg[i+1] = '\0';
            for (int j = 0; j < i+1; j++) {
                printf("%c", received_msg[j]);
            }

            // printf("> %s\n", conv_char(received_msg, ascii_msg_size, ascii_msg));

            // if exit sequence received, then exit. else, wait for next set of bits after synchronisation
            if (break_val) break;
        }
    }

    // int ascii_msg_size = received_msg_size / 8;
    // char ascii_msg[ascii_msg_size];
    // for (int j = 0; j < ascii_msg_size; j++) {
    //     char tmp[8];
    //     int k = 0;
    //     for (int l = j * 8; l < ((j + 1) * 8); l++) {
    //         tmp[k++] = received_msg[l];
    //     }
    //     char tm = strtol(tmp, 0, 2);
    //     ascii_msg[j] = tm;
    // }
    // ascii_msg[ascii_msg_size] = '\0';

    unmap_file(handle);

    printf("[Receiver] File received : %u bytes\n", received_msg_size);
    printf("%s\n", binary_to_ascii(received_msg));

    // DO NOT MODIFY THIS LINE
    printf("Accuracy (%%): %f\n", check_accuracy(received_msg, received_msg_size)*100);
}
#include <stdio.h>
#include "utils.h"

int main() {

    // Update these values accordingly
    char received_msg[MSG_SIZE];
    int received_msg_size = 0;

    // declare handle for file mapping
    map_handle_t *handle;

    // map file in memory
    map_file(SHARED_MEMORY_OBJECT, &handle);

    // receive file contents
    uint32_t sequenceMask = ((uint32_t) 1 << 6) - 1;
    uint32_t expSequence = 0b101110;
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
            for (i = 0; i < MSG_SIZE/8; i++) {
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
            // received_msg[i+1] = '\0';
            // printf("%s\n", binary_to_string(received_msg));

            // if exit sequence received, then exit. else, wait for next set of bits after synchronisation
            if (break_val) break;
        }
    }

    received_msg[i-16] = '\0';
    received_msg_size -= 2;
    printf("%s\n", binary_to_string(received_msg));

    printf("[Receiver] File received : %u bytes\n", received_msg_size);
    // printf("%s\n", binary_to_string(received_msg));

    write_binary_comparison_to_file(received_msg);
    unmap_file(handle);

    // DO NOT MODIFY THIS LINE
    printf("Accuracy in %%: %0.2f\n", check_accuracy(received_msg, received_msg_size)*100);
}
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include "utils.h"

int main() {

    // ********** DO NOT MODIFY THIS SECTION **********
    FILE *fp = fopen(MSG_FILE, "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    char msg[MAX_MSG_SIZE];
    int msg_size = 0;
    char c;
    while ((c = fgetc(fp)) != EOF) {
        msg[msg_size++] = c;
    }
    fclose(fp);

    clock_t start = clock();
    // unsigned long long start = rdtsc();
    // **********************************************
    // ********** YOUR CODE STARTS HERE **********

    // declare handle for file mapping
    map_handle_t *handle;

    // map file in memory
    // char *map = (char *) map_file("msg.txt", &handle);
    map_file("msg.txt", &handle);

    // open file
    FILE *fptr;
    fptr = fopen("msg.txt", "r");
    if (fptr == NULL) {
        printf("Could not open %s\n", "msg.txt");
        exit(0);
    }

    // wait for receiver to be ready
    io_sync();

    // send file contents
    char *binary = string_to_binary(msg);
    unsigned int bin_len = strlen(binary);

    bool sequence[8] = {1, 0, 1, 0, 1, 0, 1, 1};
    int idx = 0;
    bool done = false;

    // send the sequence
    while (!done) {
        for (int i = 0; i < 8; i++) {
            send_bit(sequence[i], handle);
        }

        // wait for receiver to be ready
        io_sync();

        // send data
        for (int j = 0; j < MAX_MSG_SIZE / 8; j++) {
            if (binary[idx] == '0') {
                send_bit(false, handle);
            } else {
                send_bit(true, handle);
            }
            idx++;
            if (idx == bin_len) {
                done = true;
                break;
            }
        }

        // send zeros when done to ensure channel is closed
        if (done) {
            for (int i = 0; i < 32; i++) {
                send_bit(false, handle);
            }
        }

        // wait for receiver to be ready
        io_sync();
    }

    // close channel
    io_sync();
    for (int i = 0; i < 8; i++) {
        send_bit(sequence[i], handle);
    }
    for (int i = 0; i < 32; i++) {
        send_bit(false, handle);
    }

    // print file data to stdout
    printf("%s\n", msg);

    // unmap file
    unmap_file(handle);
    // close file
    fclose(fptr);

    // ********** YOUR CODE ENDS HERE **********
    // ********** DO NOT MODIFY THIS SECTION **********
    clock_t end = clock();
    // unsigned long long end = rdtsc();
    double time_taken = ((double)end - start) / CLOCKS_PER_SEC;
    printf("Message sent successfully\n");
    printf("Time taken to send the message: %f\n", time_taken);
    printf("Message size: %d\n", msg_size);
    printf("Bits per second: %f\n", msg_size * 8 / time_taken);
    // **********************************************
}

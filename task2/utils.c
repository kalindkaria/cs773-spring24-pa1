#include "utils.h"

void* map_file (const char* filename, map_handle_t** handle) {
    if (filename == NULL)
        return NULL;

    *handle = calloc(1, sizeof(map_handle_t));
    if (*handle == NULL)
        return NULL;

    (*handle)->fd = open(filename, O_RDONLY);
    if ((*handle)->fd == -1)
        return NULL;

    struct stat filestat;
    if (fstat((*handle)->fd, &filestat) == -1) {
        close((*handle)->fd);
        return NULL;
    }

    (*handle)->range = filestat.st_size;

    (*handle)->mapping = mmap(0, (*handle)->range, PROT_READ, MAP_SHARED, (*handle)->fd, 0);

    return (*handle)->mapping;
}

void unmap_file (map_handle_t* handle) {
    if (handle == NULL)
        return;

    munmap(handle->mapping, handle->range);
    close(handle->fd);

    free(handle);
}

extern inline __attribute((always_inline))
uint64_t rdtsc() {
	uint64_t a, d;
	asm volatile ("mfence");
	asm volatile ("rdtsc" : "=a" (a), "=d" (d));
	a = (d<<32) | a;
	asm volatile ("mfence");
	return a;
}

extern inline __attribute((always_inline))
void clflush(void* addr) {
    asm volatile ("clflush 0(%0)\n"
    :
    : "c" (addr)
    : "rax");
}

extern inline __attribute((always_inline))
uint64_t probe_timing(char *addr) {
    volatile uint64_t time;

    // asm __volatile__(
    //     "    mfence             \n"
    //     "    lfence             \n"
    //     "    rdtsc              \n"
    //     "    lfence             \n"
    //     "    movl %%eax, %%esi  \n"
    //     "    movl (%1), %%eax   \n"
    //     "    lfence             \n"
    //     "    rdtsc              \n"
    //     "    subl %%esi, %%eax  \n"
    //     "    clflush 0(%1)      \n"
    //     : "=a" (time)
    //     : "c" (addr)
    //     : "%esi", "%edx"
    // );

    asm volatile(
        " mfence           \n\t"
        " lfence           \n\t"
        " rdtsc            \n\t"
        " lfence            \n\t"
        " mov %%rax, %%rdi \n\t"
        " mov (%1), %%r8   \n\t"
        " lfence           \n\t"
        " rdtsc            \n\t"
        " sub %%rdi, %%rax \n\t"
        : "=a"(time)            /*output*/
        : "c"(addr)
        : "r8", "rdi"
    );
    return time;
}

extern inline __attribute((always_inline))
void maccess(void* addr) {
    asm volatile (
    " movq (%0), %%rax  \n\t"
    :
    : "c"(addr)
    : "rax");
}

extern inline __attribute((always_inline))
uint64_t cc_sync() {
    while ((rdtsc() & CHANNEL_SYNC_TIMEMASK) > CHANNEL_SYNC_JITTER) {}
    return rdtsc();
}

extern inline __attribute((always_inline))
uint64_t io_sync() {
    while ((rdtsc() & GLOBAL_SYNC_TIMEMASK) > CHANNEL_SYNC_JITTER) {}
    return rdtsc();
}

char *string_to_binary(char *s) {
    if (s == NULL) return 0; /* no input string */
    size_t len = strlen(s);

    // each char is one byte (8 bits) and + 1 at the end for null terminator
    char *binary = malloc(len * 8 + 1);
    binary[0] = '\0';

    for (size_t i = 0; i < len; ++i) {
        char ch = s[i];
        for (int j = 7; j >= 0; --j) {
            if (ch & (1 << j)) {
                strcat(binary, "1");
            } else {
                strcat(binary, "0");
            }
        }
    }
    return binary;
}

int write_binary_comparison_to_file(char* input_binary) {
    FILE *fp = fopen(MSG_FILE, "r");
    if (fp == NULL) {
        printf("Error opening file\n");
        return 1;
    }

    char msg[MSG_SIZE];
    int msg_size = 0;
    char c;
    while ((c = fgetc(fp)) != EOF) {
        msg[msg_size++] = c;
    }
    fclose(fp);

    char* predefined_string = msg;

    // Convert predefined string to binary
    char* predefined_binary = string_to_binary(predefined_string);
    if (predefined_binary == NULL) {
        return -1;  // Memory allocation failed
    }

    // Open file for writing
    fp = fopen("binary_comparison.txt", "w");
    if (!fp) {
        free(predefined_binary);
        return -2;  // File opening failed
    }

    // Get lengths of both binary strings
    int input_len = strlen(input_binary);
    int predefined_len = strlen(predefined_binary);
    int max_len = (input_len > predefined_len) ? input_len : predefined_len;

    // Write header to file
    fprintf(fp, "Input Binary\tPredefined Binary\n");
    fprintf(fp, "-----------\t----------------\n");

    // Write bits side by side until both strings are exhausted
    for (int i = 0; i < max_len; i++) {
        char input_bit = (i < input_len) ? input_binary[i] : '-';
        char pred_bit = (i < predefined_len) ? predefined_binary[i] : '-';
        fprintf(fp, "%c\t\t%c\n", input_bit, pred_bit);
    }

    check_accuracy_own(input_binary, predefined_binary);
    // Clean up
    fclose(fp);
    free(predefined_binary);
    return 0;
}

// extern inline __attribute((always_inline))
char *binary_to_string(char *data) {
    size_t msg_len = strlen(data) / 8;
    char *msg = malloc(msg_len+1);

    for (int i = 0; i < msg_len; i++) {
        char tmp[8];
        int k = 0;

        for (int j = i * 8; j < ((i + 1) * 8); j++) {
            tmp[k++] = data[j];
        }

        char tm = strtol(tmp, 0, 2);
        msg[i] = tm;
    }

    msg[msg_len] = '\0';
    return msg;
}

void send_bit(bool one, map_handle_t *handle) {
    // synchronize with receiver
    uint64_t start_t = cc_sync();
    if (one) {
        // repeatedly flush addr to send 1
        while ((rdtsc() - start_t) < CHANNEL_DEFAULT_INTERVAL) {
            clflush(handle->mapping);
        }
    }
    else {
        // do nothing to send 0
        while ((rdtsc() - start_t) < CHANNEL_DEFAULT_INTERVAL) {}
    }
}

bool inline detect_bit(map_handle_t *handle) {
	int misses = 0, hits = 0;

	// Sync with sender
	uint64_t start_t = cc_sync();
	while ((rdtsc() - start_t) < CHANNEL_DEFAULT_INTERVAL) {
		// Load data from handle->mapping and measure latency
		uint64_t access_time = probe_timing(handle->mapping);

		// Count if it's a miss or hit depending on latency
		if (access_time > CACHE_MISS_LATENCY) {
			misses++;
		} else {
			hits++;
		}
	}
	return misses >= hits;
}

void check_accuracy_own(char *input_binary, char *predefined_binary) {
    int input_len = strlen(input_binary);
    int predefined_len = strlen(predefined_binary);
    // Count matching bytes
    int num_same_byte = 0;
    int num_total_byte = 0;
    int min_len = (input_len < predefined_len) ? input_len : predefined_len;

    // Process complete bytes (8 bits)
    for (int i = 0; i < min_len - 7; i += 8) {
        num_total_byte++;
        int match = 1;
        for (int j = 0; j < 8; j++) {
            if (input_binary[i + j] != predefined_binary[i + j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            num_same_byte++;
        }
    }

    // Handle remaining bits (less than 8)
    if (min_len % 8 != 0) {
        num_total_byte++;
        int remaining_bits = min_len % 8;
        int start_pos = min_len - remaining_bits;
        int match = 1;

        for (int i = 0; i < remaining_bits; i++) {
            if (input_binary[start_pos + i] != predefined_binary[start_pos + i]) {
                match = 0;
                break;
            }
        }
        if (match) {
            num_same_byte++;
        }
    }

    printf("\nMatching bytes: %d out of %d total bytes\n", num_same_byte, num_total_byte);
    printf("Our Accuracy in %%: %0.2f\n", (double)num_same_byte * 100 / num_total_byte);
}

// DO NOT MODIFY THIS FUNCTION
double check_accuracy (char* received_msg, int received_msg_size) {
    FILE *fp = fopen(MSG_FILE, "r");
    if(fp == NULL){
        printf("Error opening file\n");
        return 1;
    }

    char original_msg[MAX_MSG_SIZE];
    int original_msg_size = 0;
    char c;
    while((c = fgetc(fp)) != EOF){
        original_msg[original_msg_size++] = c;
    }
    fclose(fp);

    int min_size = received_msg_size < original_msg_size ? received_msg_size : original_msg_size;

    int error_count = (original_msg_size - min_size) * 8;
    for(int i = 0; i < min_size; i++){
        char xor_result = received_msg[i] ^ original_msg[i];
        for(int j = 0; j < 8; j++){
            if((xor_result >> j) & 1){
                error_count++;
            }
        }
    }
    return 1-(double)error_count / (original_msg_size * 8);
}
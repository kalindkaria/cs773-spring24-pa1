#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#define MSG_FILE                        "msg.txt"
#define MAX_MSG_SIZE                    500

#define SHARED_MEMORY_OBJECT            "shared_mem.txt"
#define MSG_SIZE                        16384
#define CHANNEL_DEFAULT_INTERVAL        0x00008000
#define CHANNEL_SYNC_TIMEMASK           0x000FFFFF
#define GLOBAL_SYNC_TIMEMASK            0x0000FFFF
#define CHANNEL_SYNC_JITTER             0x0600
#define CACHE_MISS_LATENCY              280

typedef struct map_handle_s {
  int fd;
  size_t range;
  void* mapping;
} map_handle_t;

void* map_file(const char* filename, map_handle_t** handle);
void unmap_file(map_handle_t* handle);

uint64_t rdtsc();
void clflush(void* addr);
uint64_t probe_timing(char *addr);
void maccess(void* addr);

uint64_t cc_sync();
uint64_t io_sync();

char *string_to_binary(char *s);
int write_binary_comparison_to_file(char* input_binary);
char *binary_to_string(char *data);
void send_bit(bool one, map_handle_t *handle);
bool detect_bit(map_handle_t *handle);

void check_accuracy_own(char *input_binary, char *predefined_binary);
double check_accuracy(char*, int);

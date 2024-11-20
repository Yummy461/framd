#include "util.h"

/*
 * Loads from virtual address addr and measures the access time.
 */
CYCLES measure_one_block_access_time(ADDR_PTR addr)
{
    CYCLES cycles_start, cycles_end;

    asm volatile (
        "lfence\n\t"         // 防止指令重排
        "rdtsc\n\t"          // 获取起始时间戳
        "mov %%eax, %%edi\n\t"
        "mov (%1), %%r8\n\t" // 从给定地址加载数据
        "lfence\n\t"         // 确保加载完成
        "rdtsc\n\t"          // 获取结束时间戳
        "sub %%edi, %%eax\n\t"
        : "=a"(cycles_start), "=r"(cycles_end)
        : "r"(addr)
        : "r8", "edi"
    );

    return cycles_end - cycles_start;
}

/*
 * Flushes the cache block accessed by a virtual address out of the cache.
 */
void clflush(ADDR_PTR addr)
{
    asm volatile ("clflush (%0)"::"r"(addr));
}

/*
 * Returns Time Stamp Counter.
 */
extern inline __attribute__((always_inline))
CYCLES rdtscp(void) {
    CYCLES cycles;
    asm volatile ("rdtscp" : "=a" (cycles));
    return cycles;
}

/*
 * Gets the value Time Stamp Counter.
 */
inline CYCLES get_time() {
    return rdtscp();
}

/*
 * Synchronizes at the overflow of a counter.
 */
extern inline __attribute__((always_inline))
CYCLES cc_sync() {
    while ((get_time() & CHANNEL_SYNC_TIMEMASK) > CHANNEL_SYNC_JITTER) {}
    return get_time();
}

/*
 * Converts a given ASCII string to a binary string.
 */
char *string_to_binary(char *s)
{
    if (s == NULL) return 0;

    size_t len = strlen(s) - 1;
    char *binary = malloc(len * 8 + 1);
    binary[0] = '\0';

    for (size_t i = 0; i < len; ++i) {
        char ch = s[i];
        for (int j = 7; j >= 0; --j) {
            strcat(binary, (ch & (1 << j)) ? "1" : "0");
        }
    }

    return binary;
}

/*
 * Converts 8-bit data stream into characters and returns.
 */
char *conv_char(char *data, int size, char *msg)
{
    for (int i = 0; i < size; i++) {
        char tmp[8];
        int k = 0;

        for (int j = i * 8; j < ((i + 1) * 8); j++) {
            tmp[k++] = data[j];
        }

        char tm = strtol(tmp, 0, 2);
        msg[i] = tm;
    }

    msg[size] = '\0';
    return msg;
}

/*
 * Prints help menu.
 */
void print_help() {
    printf("Flush+Reload Covert Channel Sender/Receiver Flags:\n"
           "\t-f,\tFile to be shared between sender/receiver\n"
           "\t-o,\tSelected offset into shared file\n"
           "\t-i,\tTime interval for sending a single bit\n");
}

/*
 * Parses the arguments and flags of the program and initializes the struct config.
 */
void init_config(struct config *config, int argc, char **argv)
{
    int offset = DEFAULT_FILE_OFFSET;
    config->interval = CHANNEL_DEFAULT_INTERVAL;
    char *filename = DEFAULT_FILE_NAME;

    int option;
    while ((option = getopt(argc, argv, "i:o:f:")) != -1) {
        switch (option) {
            case 'i':
                config->interval = atoi(optarg);
                break;
            case 'o':
                offset = atoi(optarg) * CACHE_BLOCK_SIZE;
                break;
            case 'f':
                filename = optarg;
                break;
            case 'h':
                print_help();
                exit(1);
            case '?':
                fprintf(stderr, "Unknown option character\n");
                print_help();
                exit(1);
            default:
                print_help();
                exit(1);
        }
    }

    if (filename != NULL) {
        int inFile = open(filename, O_RDONLY);
        if (inFile == -1) {
            printf("Failed to Open File\n");
            exit(1);
        }

        void *mapaddr = mmap(NULL, DEFAULT_FILE_SIZE, PROT_READ, MAP_SHARED, inFile, 0);
        if (mapaddr == (void *) -1) {
            printf("Failed to Map Address\n");
            exit(1);
        }

        config->addr = (ADDR_PTR) mapaddr + offset;
    }
}

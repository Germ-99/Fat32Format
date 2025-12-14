#include "../../include/linuxfttf.h"

void print_usage(const char *program_name) {
    printf("Usage: %s [OPTIONS] <device>\n", program_name);
    printf("\nOptions:\n");
    printf("  -f, --force     Skip confirmation prompt\n");
    printf("  -v, --verbose   Show detailed progress information\n");
    printf("  -h, --help      Display this help message\n");
    printf("\nExample:\n");
    printf("  %s /dev/sdb\n", program_name);
    printf("  %s -f -v /dev/sdc\n", program_name);
    printf("\nWARNING: This will erase all data on the specified device!\n");
}

void parse_args(int argc, char **argv, ProgramArgs *args) {
    args->device = NULL;
    args->force = 0;
    args->verbose = 0;

    if (argc < 2) {
        print_usage(argv[0]);
        exit(1);
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            exit(0);
        } else if (strcmp(argv[i], "-f") == 0 || strcmp(argv[i], "--force") == 0) {
            args->force = 1;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--verbose") == 0) {
            args->verbose = 1;
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            print_usage(argv[0]);
            exit(1);
        } else {
            if (args->device != NULL) {
                fprintf(stderr, "Error: Multiple devices specified\n");
                exit(1);
            }
            args->device = argv[i];
        }
    }

    if (args->device == NULL) {
        fprintf(stderr, "Error: No device specified\n");
        print_usage(argv[0]);
        exit(1);
    }
}
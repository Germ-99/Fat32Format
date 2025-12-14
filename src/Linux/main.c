#include "../../include/linuxfttf.h"

int main(int argc, char **argv) {
    ProgramArgs args;
    parse_args(argc, argv, &args);

    if (geteuid() != 0) {
        fprintf(stderr, "Error: This program must be run as root\n");
        fprintf(stderr, "Try: sudo %s %s\n", argv[0], args.device);
        return 1;
    }

    struct stat st;
    if (stat(args.device, &st) == -1) {
        fprintf(stderr, "Error: Device %s does not exist\n", args.device);
        return 1;
    }

    if (!S_ISBLK(st.st_mode)) {
        fprintf(stderr, "Error: %s is not a block device\n", args.device);
        return 1;
    }

    DriveInfo *drives = NULL;
    int drive_count = enumerate_drives(&drives);
    
    int is_valid_drive = 0;
    uint64_t device_size = 0;
    char device_model[256] = "Unknown";

    for (int i = 0; i < drive_count; i++) {
        if (strcmp(drives[i].path, args.device) == 0) {
            is_valid_drive = 1;
            device_size = drives[i].size;
            size_t len = strlen(drives[i].model);
            if (len >= sizeof(device_model)) {
                len = sizeof(device_model) - 1;
            }
            memcpy(device_model, drives[i].model, len);
            device_model[len] = '\0';
            break;
        }
    }

    free_drives(drives);

    if (!is_valid_drive) {
        fprintf(stderr, "Warning: %s may not be a removable drive\n", args.device);
    }

    if (is_drive_mounted(args.device)) {
        printf("Device %s is mounted. Attempting to unmount...\n", args.device);
        if (!unmount_drive(args.device)) {
            fprintf(stderr, "Error: Failed to unmount all partitions\n");
            fprintf(stderr, "Please manually unmount the device before formatting\n");
            return 1;
        }
        sleep(1);
    }

    if (!args.force) {
        printf("\n");
        printf("WARNING: This will DESTROY ALL DATA on %s!\n", args.device);
        if (device_size > 0) {
            printf("Device: %s (%.2f GB)\n", device_model, 
                   (double)device_size / (1024.0 * 1024.0 * 1024.0));
        }
        printf("\n");
        printf("Type 'YES' to continue: ");
        fflush(stdout);

        char response[16];
        if (fgets(response, sizeof(response), stdin) == NULL) {
            printf("\nOperation cancelled\n");
            return 1;
        }

        response[strcspn(response, "\n")] = 0;

        if (strcmp(response, "YES") != 0) {
            printf("Operation cancelled\n");
            return 1;
        }
    }

    printf("\nFormatting %s as FAT32...\n", args.device);

    if (!format_drive(args.device, args.verbose)) {
        fprintf(stderr, "\nFormat failed!\n");
        return 1;
    }

    printf("\nFormat completed successfully!\n");
    printf("You may need to re-plug the device for changes to take effect.\n");

    return 0;
}
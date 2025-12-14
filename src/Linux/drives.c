#include "../../include/linuxfttf.h"

int is_drive_mounted(const char *device) {
    FILE *mtab = setmntent("/proc/mounts", "r");
    if (!mtab) {
        return 0;
    }

    struct mntent *entry;
    int mounted = 0;

    while ((entry = getmntent(mtab)) != NULL) {
        if (strncmp(entry->mnt_fsname, device, strlen(device)) == 0) {
            mounted = 1;
            break;
        }
    }

    endmntent(mtab);
    return mounted;
}

int unmount_drive(const char *device) {
    int unmounted_any = 0;

    FILE *mtab = setmntent("/proc/mounts", "r");
    if (!mtab) {
        return 0;
    }

    struct mntent *entry;
    char **mount_list = NULL;
    int mount_count = 0;

    while ((entry = getmntent(mtab)) != NULL) {
        if (strncmp(entry->mnt_fsname, device, strlen(device)) == 0) {
            mount_list = realloc(mount_list, (mount_count + 1) * sizeof(char*));
            mount_list[mount_count] = strdup(entry->mnt_fsname);
            mount_count++;
        }
    }

    endmntent(mtab);

    for (int i = 0; i < mount_count; i++) {
        printf("Unmounting %s...\n", mount_list[i]);
        if (umount2(mount_list[i], MNT_FORCE) == 0) {
            unmounted_any = 1;
        } else {
            fprintf(stderr, "Warning: Failed to unmount %s: %s\n", 
                    mount_list[i], strerror(errno));
        }
        free(mount_list[i]);
    }

    if (mount_list) {
        free(mount_list);
    }

    return unmounted_any;
}

int enumerate_drives(DriveInfo **drives) {
    *drives = NULL;
    int count = 0;
    int capacity = 4;

    *drives = malloc(capacity * sizeof(DriveInfo));
    if (!*drives) {
        return 0;
    }

    FILE *fp = fopen("/proc/partitions", "r");
    if (!fp) {
        free(*drives);
        *drives = NULL;
        return 0;
    }

    char line[256];
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return count;
    }
    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return count;
    }

    while (fgets(line, sizeof(line), fp)) {
        unsigned long major, minor, blocks;
        char name[64];

        if (sscanf(line, "%lu %lu %lu %s", &major, &minor, &blocks, name) != 4) {
            continue;
        }

        if (strchr(name, '1') || strchr(name, '2') || strchr(name, '3') || 
            strchr(name, '4') || strchr(name, '5') || strchr(name, '6') ||
            strchr(name, '7') || strchr(name, '8') || strchr(name, '9')) {
            continue;
        }

        if (strncmp(name, "loop", 4) == 0 || strncmp(name, "ram", 3) == 0) {
            continue;
        }

        char removable_path[512];
        snprintf(removable_path, sizeof(removable_path), "/sys/block/%s/removable", name);
        
        FILE *rem_fp = fopen(removable_path, "r");
        int removable = 0;
        if (rem_fp) {
            char rem_val[8];
            if (fgets(rem_val, sizeof(rem_val), rem_fp)) {
                removable = atoi(rem_val);
            }
            fclose(rem_fp);
        }

        if (count >= capacity) {
            capacity *= 2;
            DriveInfo *temp = realloc(*drives, capacity * sizeof(DriveInfo));
            if (!temp) {
                free(*drives);
                *drives = NULL;
                fclose(fp);
                return 0;
            }
            *drives = temp;
        }

        DriveInfo *info = &(*drives)[count];
        snprintf(info->path, sizeof(info->path), "/dev/%s", name);
        info->size = (uint64_t)blocks * 1024;
        info->removable = removable;

        char model_path[512];
        snprintf(model_path, sizeof(model_path), "/sys/block/%s/device/model", name);
        FILE *model_fp = fopen(model_path, "r");
        if (model_fp) {
            if (fgets(info->model, sizeof(info->model), model_fp)) {
                info->model[strcspn(info->model, "\n")] = 0;
                
                int len = strlen(info->model);
                while (len > 0 && isspace(info->model[len - 1])) {
                    info->model[--len] = 0;
                }
            } else {
                strcpy(info->model, "Unknown");
            }
            fclose(model_fp);
        } else {
            strcpy(info->model, "Unknown");
        }

        count++;
    }

    fclose(fp);
    return count;
}

void free_drives(DriveInfo *drives) {
    if (drives) {
        free(drives);
    }
}
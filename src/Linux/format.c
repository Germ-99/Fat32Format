#include "../../include/linuxfttf.h"

uint64_t get_device_size(int fd) {
    uint64_t size;
    if (ioctl(fd, BLKGETSIZE64, &size) == -1) {
        return 0;
    }
    return size;
}

int calculate_fat32_parameters(uint64_t diskSize, uint32_t bytesPerSector, FAT32Params *params) {
    params->bytesPerSector = bytesPerSector;
    params->totalSectors = (uint32_t)(diskSize / bytesPerSector);
    params->reservedSectors = 32;
    params->numberOfFATs = 2;
    params->rootDirFirstCluster = 2;

    if (diskSize < 512ULL * 1024 * 1024) {
        params->sectorsPerCluster = 1;
    } else if (diskSize < 8ULL * 1024 * 1024 * 1024) {
        params->sectorsPerCluster = 8;
    } else if (diskSize < 16ULL * 1024 * 1024 * 1024) {
        params->sectorsPerCluster = 16;
    } else if (diskSize < 32ULL * 1024 * 1024 * 1024) {
        params->sectorsPerCluster = 32;
    } else {
        params->sectorsPerCluster = 64;
    }

    uint32_t tmpVal1 = params->totalSectors - params->reservedSectors;
    uint32_t tmpVal2 = (256 * params->sectorsPerCluster) + params->numberOfFATs;
    tmpVal2 = tmpVal2 / 2;
    params->sectorsPerFAT = (tmpVal1 + tmpVal2 - 1) / tmpVal2;

    return 1;
}

int write_fat32_structures(int fd, FAT32Params *params, int verbose) {
    if (lseek(fd, 0, SEEK_SET) == -1) {
        return 0;
    }

    if (verbose) {
        printf("Writing boot sector...\n");
    }

    FAT32BootSector bootSector;
    memset(&bootSector, 0, sizeof(bootSector));

    bootSector.jmpBoot[0] = 0xEB;
    bootSector.jmpBoot[1] = 0x58;
    bootSector.jmpBoot[2] = 0x90;

    memcpy(bootSector.OEMName, "MSWIN4.1", 8);
    bootSector.bytesPerSector = (uint16_t)params->bytesPerSector;
    bootSector.sectorsPerCluster = (uint8_t)params->sectorsPerCluster;
    bootSector.reservedSectorCount = (uint16_t)params->reservedSectors;
    bootSector.numFATs = params->numberOfFATs;
    bootSector.rootEntryCount = 0;
    bootSector.totalSectors16 = 0;
    bootSector.media = 0xF8;
    bootSector.FATSize16 = 0;
    bootSector.sectorsPerTrack = 63;
    bootSector.numberHeads = 255;
    bootSector.hiddenSectors = 0;
    bootSector.totalSectors32 = params->totalSectors;
    bootSector.FATSize32 = params->sectorsPerFAT;
    bootSector.extFlags = 0;
    bootSector.FSVersion = 0;
    bootSector.rootCluster = params->rootDirFirstCluster;
    bootSector.FSInfoSector = 1;
    bootSector.backupBootSector = 6;
    bootSector.driveNumber = 0x80;
    bootSector.bootSignature = 0x29;
    bootSector.volumeID = (uint32_t)time(NULL);
    memcpy(bootSector.volumeLabel, "NO NAME    ", 11);
    memcpy(bootSector.fileSystemType, "FAT32   ", 8);

    uint8_t *sector = malloc(params->bytesPerSector);
    if (!sector) {
        return 0;
    }

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &bootSector, sizeof(bootSector));
    sector[510] = 0x55;
    sector[511] = 0xAA;

    if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
        free(sector);
        return 0;
    }

    if (verbose) {
        printf("Writing FSInfo sector...\n");
    }

    FSInfoSector fsInfo;
    memset(&fsInfo, 0, sizeof(fsInfo));
    fsInfo.leadSignature = 0x41615252;
    fsInfo.structSignature = 0x61417272;
    fsInfo.freeCount = 0xFFFFFFFF;
    fsInfo.nextFree = 0xFFFFFFFF;
    fsInfo.trailSignature = 0xAA550000;

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &fsInfo, sizeof(fsInfo));

    if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
        free(sector);
        return 0;
    }

    memset(sector, 0, params->bytesPerSector);
    for (int i = 0; i < 4; i++) {
        if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
            free(sector);
            return 0;
        }
    }

    if (verbose) {
        printf("Writing backup boot sector...\n");
    }

    if (lseek(fd, 6 * params->bytesPerSector, SEEK_SET) == -1) {
        free(sector);
        return 0;
    }

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &bootSector, sizeof(bootSector));
    sector[510] = 0x55;
    sector[511] = 0xAA;
    
    if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
        free(sector);
        return 0;
    }

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &fsInfo, sizeof(fsInfo));
    
    if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
        free(sector);
        return 0;
    }

    if (verbose) {
        printf("Writing FAT tables...\n");
    }

    if (lseek(fd, params->reservedSectors * params->bytesPerSector, SEEK_SET) == -1) {
        free(sector);
        return 0;
    }

    for (int fat = 0; fat < params->numberOfFATs; fat++) {
        memset(sector, 0, params->bytesPerSector);
        
        uint32_t *fatEntries = (uint32_t*)sector;
        fatEntries[0] = 0x0FFFFFF8;
        fatEntries[1] = 0x0FFFFFFF;
        fatEntries[2] = 0x0FFFFFFF;

        if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
            free(sector);
            return 0;
        }

        memset(sector, 0, params->bytesPerSector);
        for (uint32_t i = 1; i < params->sectorsPerFAT; i++) {
            if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
                free(sector);
                return 0;
            }
        }
    }

    if (verbose) {
        printf("Clearing root directory...\n");
    }

    uint32_t rootDirSector = params->reservedSectors + 
                             (params->numberOfFATs * params->sectorsPerFAT) +
                             ((params->rootDirFirstCluster - 2) * params->sectorsPerCluster);
    
    if (lseek(fd, rootDirSector * params->bytesPerSector, SEEK_SET) == -1) {
        free(sector);
        return 0;
    }

    memset(sector, 0, params->bytesPerSector);
    for (uint32_t i = 0; i < params->sectorsPerCluster; i++) {
        if (write(fd, sector, params->bytesPerSector) != params->bytesPerSector) {
            free(sector);
            return 0;
        }
    }

    free(sector);
    fsync(fd);

    return 1;
}

int format_drive(const char *device, int verbose) {
    if (verbose) {
        printf("Opening device %s...\n", device);
    }

    int fd = open(device, O_RDWR);
    if (fd == -1) {
        fprintf(stderr, "Error: Failed to open device %s: %s\n", device, strerror(errno));
        return 0;
    }

    uint64_t diskSize = get_device_size(fd);
    if (diskSize == 0) {
        fprintf(stderr, "Error: Failed to get device size\n");
        close(fd);
        return 0;
    }

    if (verbose) {
        printf("Device size: %.2f GB\n", (double)diskSize / (1024.0 * 1024.0 * 1024.0));
    }

    FAT32Params params;
    if (!calculate_fat32_parameters(diskSize, SECTOR_SIZE, &params)) {
        fprintf(stderr, "Error: Failed to calculate FAT32 parameters\n");
        close(fd);
        return 0;
    }

    if (verbose) {
        printf("Sectors per cluster: %u\n", params.sectorsPerCluster);
        printf("Total sectors: %u\n", params.totalSectors);
        printf("Sectors per FAT: %u\n", params.sectorsPerFAT);
    }

    if (!write_fat32_structures(fd, &params, verbose)) {
        fprintf(stderr, "Error: Failed to write FAT32 structures\n");
        close(fd);
        return 0;
    }

    close(fd);

    if (verbose) {
        printf("Syncing filesystem...\n");
    }
    sync();

    return 1;
}
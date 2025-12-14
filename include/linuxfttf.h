#ifndef LINUXFTTF_H
#define LINUXFTTF_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <linux/hdreg.h>
#include <errno.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <mntent.h>
#include <time.h>

#define SECTOR_SIZE 512

typedef struct {
    char path[256];
    char model[256];
    uint64_t size;
    int removable;
} DriveInfo;

typedef struct {
    uint32_t sectorsPerCluster;
    uint32_t bytesPerSector;
    uint32_t reservedSectors;
    uint8_t numberOfFATs;
    uint32_t sectorsPerFAT;
    uint32_t totalSectors;
    uint32_t rootDirFirstCluster;
} FAT32Params;

typedef struct {
    uint8_t jmpBoot[3];
    uint8_t OEMName[8];
    uint16_t bytesPerSector;
    uint8_t sectorsPerCluster;
    uint16_t reservedSectorCount;
    uint8_t numFATs;
    uint16_t rootEntryCount;
    uint16_t totalSectors16;
    uint8_t media;
    uint16_t FATSize16;
    uint16_t sectorsPerTrack;
    uint16_t numberHeads;
    uint32_t hiddenSectors;
    uint32_t totalSectors32;
    uint32_t FATSize32;
    uint16_t extFlags;
    uint16_t FSVersion;
    uint32_t rootCluster;
    uint16_t FSInfoSector;
    uint16_t backupBootSector;
    uint8_t reserved[12];
    uint8_t driveNumber;
    uint8_t reserved1;
    uint8_t bootSignature;
    uint32_t volumeID;
    uint8_t volumeLabel[11];
    uint8_t fileSystemType[8];
} __attribute__((packed)) FAT32BootSector;

typedef struct {
    uint32_t leadSignature;
    uint8_t reserved1[480];
    uint32_t structSignature;
    uint32_t freeCount;
    uint32_t nextFree;
    uint8_t reserved2[12];
    uint32_t trailSignature;
} __attribute__((packed)) FSInfoSector;

typedef struct {
    char *device;
    int force;
    int verbose;
} ProgramArgs;

void parse_args(int argc, char **argv, ProgramArgs *args);
void print_usage(const char *program_name);

int enumerate_drives(DriveInfo **drives);
void free_drives(DriveInfo *drives);
int is_drive_mounted(const char *device);
int unmount_drive(const char *device);

int format_drive(const char *device, int verbose);
int calculate_fat32_parameters(uint64_t diskSize, uint32_t bytesPerSector, FAT32Params *params);
int write_fat32_structures(int fd, FAT32Params *params, int verbose);
uint64_t get_device_size(int fd);

#endif
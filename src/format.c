#include "../include/fttf.h"

#pragma pack(push, 1)
typedef struct {
    BYTE jmpBoot[3];
    BYTE OEMName[8];
    WORD bytesPerSector;
    BYTE sectorsPerCluster;
    WORD reservedSectorCount;
    BYTE numFATs;
    WORD rootEntryCount;
    WORD totalSectors16;
    BYTE media;
    WORD FATSize16;
    WORD sectorsPerTrack;
    WORD numberHeads;
    DWORD hiddenSectors;
    DWORD totalSectors32;
    DWORD FATSize32;
    WORD extFlags;
    WORD FSVersion;
    DWORD rootCluster;
    WORD FSInfoSector;
    WORD backupBootSector;
    BYTE reserved[12];
    BYTE driveNumber;
    BYTE reserved1;
    BYTE bootSignature;
    DWORD volumeID;
    BYTE volumeLabel[11];
    BYTE fileSystemType[8];
} FAT32BootSector;

typedef struct {
    DWORD leadSignature;
    BYTE reserved1[480];
    DWORD structSignature;
    DWORD freeCount;
    DWORD nextFree;
    BYTE reserved2[12];
    DWORD trailSignature;
} FSInfoSector;
#pragma pack(pop)

BOOL FormatDrive(WCHAR driveLetter, HWND hwnd) {
    WCHAR volumePath[MAX_PATH];
    swprintf(volumePath, MAX_PATH, L"\\\\.\\%c:", driveLetter);

    LogMessage(hwnd, L"Opening drive %c:...\r\n", driveLetter);

    HANDLE hDrive = CreateFileW(
        volumePath,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        0,
        NULL
    );

    if (hDrive == INVALID_HANDLE_VALUE) {
        LogMessage(hwnd, L"Failed to open drive (error %lu)\r\n", GetLastError());
        return FALSE;
    }

    LogMessage(hwnd, L"Locking volume...\r\n");
    if (!LockVolume(hDrive)) {
        LogMessage(hwnd, L"Failed to lock volume\r\n");
        CloseHandle(hDrive);
        return FALSE;
    }

    LogMessage(hwnd, L"Dismounting volume...\r\n");
    if (!DismountVolume(hDrive)) {
        LogMessage(hwnd, L"Warning: Could not dismount volume\r\n");
    }

    DISK_GEOMETRY diskGeometry;
    DWORD bytesReturned;
    
    if (!DeviceIoControl(hDrive, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0,
                         &diskGeometry, sizeof(diskGeometry), &bytesReturned, NULL)) {
        LogMessage(hwnd, L"Failed to get disk geometry\r\n");
        CloseHandle(hDrive);
        return FALSE;
    }

    ULONGLONG diskSize = (ULONGLONG)diskGeometry.Cylinders.QuadPart *
                         diskGeometry.TracksPerCylinder *
                         diskGeometry.SectorsPerTrack *
                         diskGeometry.BytesPerSector;

    LogMessage(hwnd, L"Disk size: %.2f GB\r\n", (double)diskSize / (1024.0 * 1024.0 * 1024.0));
    LogMessage(hwnd, L"Bytes per sector: %lu\r\n", diskGeometry.BytesPerSector);

    FAT32Params params;
    if (!CalculateFAT32Parameters(diskSize, diskGeometry.BytesPerSector, &params)) {
        LogMessage(hwnd, L"Couldn't calculate FAT32 params\r\n");
        CloseHandle(hDrive);
        return FALSE;
    }

    LogMessage(hwnd, L"Sectors per cluster: %lu\r\n", params.sectorsPerCluster);
    LogMessage(hwnd, L"Total sectors: %lu\r\n", params.totalSectors);
    LogMessage(hwnd, L"Sectors per FAT: %lu\r\n", params.sectorsPerFAT);

    if (!WriteFAT32Structures(hDrive, &params, hwnd)) {
        LogMessage(hwnd, L"Failed to write FAT32 structures\r\n");
        CloseHandle(hDrive);
        return FALSE;
    }

    CloseHandle(hDrive);
    
    LogMessage(hwnd, L"Refreshing drive...\r\n");
    WCHAR driveRoot[4] = {driveLetter, L':', L'\\', 0};
    SHChangeNotify(SHCNE_DRIVEADD, SHCNF_PATH, driveRoot, NULL);

    return TRUE;
}

BOOL CalculateFAT32Parameters(ULONGLONG diskSize, DWORD bytesPerSector, FAT32Params* params) {
    params->bytesPerSector = bytesPerSector;
    params->totalSectors = (DWORD)(diskSize / bytesPerSector);
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

    DWORD tmpVal1 = params->totalSectors - params->reservedSectors;
    DWORD tmpVal2 = (256 * params->sectorsPerCluster) + params->numberOfFATs;
    tmpVal2 = tmpVal2 / 2;
    params->sectorsPerFAT = (tmpVal1 + tmpVal2 - 1) / tmpVal2;

    return TRUE;
}

BOOL WriteFAT32Structures(HANDLE hDrive, FAT32Params* params, HWND hwnd) {
    LARGE_INTEGER pos;
    pos.QuadPart = 0;
    if (!SetFilePointerEx(hDrive, pos, NULL, FILE_BEGIN)) {
        return FALSE;
    }

    LogMessage(hwnd, L"Writing boot sector...\r\n");

    FAT32BootSector bootSector;
    memset(&bootSector, 0, sizeof(bootSector));

    bootSector.jmpBoot[0] = 0xEB;
    bootSector.jmpBoot[1] = 0x58;
    bootSector.jmpBoot[2] = 0x90;

    memcpy(bootSector.OEMName, "MSWIN4.1", 8);
    bootSector.bytesPerSector = (WORD)params->bytesPerSector;
    bootSector.sectorsPerCluster = (BYTE)params->sectorsPerCluster;
    bootSector.reservedSectorCount = (WORD)params->reservedSectors;
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
    bootSector.volumeID = GetTickCount();
    memcpy(bootSector.volumeLabel, "NO NAME    ", 11);
    memcpy(bootSector.fileSystemType, "FAT32   ", 8);

    BYTE* sector = (BYTE*)malloc(params->bytesPerSector);
    if (!sector) {
        return FALSE;
    }

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &bootSector, sizeof(bootSector));
    sector[510] = 0x55;
    sector[511] = 0xAA;

    DWORD written;
    if (!WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL)) {
        free(sector);
        return FALSE;
    }

    LogMessage(hwnd, L"Writing FSInfo sector...\r\n");

    FSInfoSector fsInfo;
    memset(&fsInfo, 0, sizeof(fsInfo));
    fsInfo.leadSignature = 0x41615252;
    fsInfo.structSignature = 0x61417272;
    fsInfo.freeCount = 0xFFFFFFFF;
    fsInfo.nextFree = 0xFFFFFFFF;
    fsInfo.trailSignature = 0xAA550000;

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &fsInfo, sizeof(fsInfo));

    if (!WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL)) {
        free(sector);
        return FALSE;
    }

    pos.QuadPart = 2 * params->bytesPerSector;
    SetFilePointerEx(hDrive, pos, NULL, FILE_BEGIN);
    memset(sector, 0, params->bytesPerSector);
    for (int i = 0; i < 4; i++) {
        WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL);
    }

    LogMessage(hwnd, L"Writing backup boot sector...\r\n");

    pos.QuadPart = 6 * params->bytesPerSector;
    SetFilePointerEx(hDrive, pos, NULL, FILE_BEGIN);
    
    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &bootSector, sizeof(bootSector));
    sector[510] = 0x55;
    sector[511] = 0xAA;
    WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL);

    memset(sector, 0, params->bytesPerSector);
    memcpy(sector, &fsInfo, sizeof(fsInfo));
    WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL);

    LogMessage(hwnd, L"Writing FAT tables...\r\n");

    pos.QuadPart = params->reservedSectors * params->bytesPerSector;
    SetFilePointerEx(hDrive, pos, NULL, FILE_BEGIN);

    for (int fat = 0; fat < params->numberOfFATs; fat++) {
        memset(sector, 0, params->bytesPerSector);
        
        DWORD* fatEntries = (DWORD*)sector;
        fatEntries[0] = 0x0FFFFFF8;
        fatEntries[1] = 0x0FFFFFFF;
        fatEntries[2] = 0x0FFFFFFF;

        WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL);

        memset(sector, 0, params->bytesPerSector);
        for (DWORD i = 1; i < params->sectorsPerFAT; i++) {
            WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL);
        }
    }

    LogMessage(hwnd, L"Clearing root directory...\r\n");

    DWORD rootDirSector = params->reservedSectors + 
                          (params->numberOfFATs * params->sectorsPerFAT) +
                          ((params->rootDirFirstCluster - 2) * params->sectorsPerCluster);
    
    pos.QuadPart = rootDirSector * params->bytesPerSector;
    SetFilePointerEx(hDrive, pos, NULL, FILE_BEGIN);

    memset(sector, 0, params->bytesPerSector);
    for (DWORD i = 0; i < params->sectorsPerCluster; i++) {
        WriteFile(hDrive, sector, params->bytesPerSector, &written, NULL);
    }

    free(sector);

    FlushFileBuffers(hDrive);

    return TRUE;
}

BOOL LockVolume(HANDLE hDrive) {
    DWORD bytesReturned;
    for (int i = 0; i < 10; i++) {
        if (DeviceIoControl(hDrive, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL)) {
            return TRUE;
        }
        Sleep(500);
    }
    return FALSE;
}

BOOL UnmountVolume(HANDLE hDrive) {
    DWORD bytesReturned;
    return DeviceIoControl(hDrive, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &bytesReturned, NULL);
}

BOOL DismountVolume(HANDLE hDrive) {
    return UnmountVolume(hDrive);
}
#include "../include/fttf.h"

int EnumerateDrives(DriveInfo** drives) {
    *drives = NULL;
    int count = 0;
    int capacity = 4;
    
    *drives = (DriveInfo*)malloc(capacity * sizeof(DriveInfo));
    if (!*drives) {
        return 0;
    }

    DWORD driveMask = GetLogicalDrives();
    
    for (int i = 0; i < 26; i++) {
        if (!(driveMask & (1 << i))) {
            continue;
        }

        WCHAR drivePath[4] = {L'A' + i, L':', L'\\', 0};
        UINT driveType = GetDriveTypeW(drivePath);

        if (driveType != DRIVE_REMOVABLE) {
            continue;
        }

        if (count >= capacity) {
            capacity *= 2;
            DriveInfo* temp = (DriveInfo*)realloc(*drives, capacity * sizeof(DriveInfo));
            if (!temp) {
                FreeDrives(*drives);
                *drives = NULL;
                return 0;
            }
            *drives = temp;
        }

        DriveInfo* info = &(*drives)[count];
        info->letter = L'A' + i;
        info->label[0] = 0;
        info->totalSize = 0;
        info->freeSize = 0;

        WCHAR volumePath[MAX_PATH];
        swprintf(volumePath, MAX_PATH, L"%c:\\", info->letter);

        WCHAR volumeName[MAX_PATH] = {0};
        GetVolumeInformationW(volumePath, volumeName, MAX_PATH, NULL, NULL, NULL, NULL, 0);
        wcsncpy_s(info->label, 256, volumeName, 255);
        info->label[255] = 0;

        ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
        if (GetDiskFreeSpaceExW(volumePath, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
            info->totalSize = totalBytes.QuadPart;
            info->freeSize = totalFreeBytes.QuadPart;
        }

        count++;
    }

    return count;
}

void FreeDrives(DriveInfo* drives) {
    if (drives) {
        free(drives);
    }
}
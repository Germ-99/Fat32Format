#ifndef FTTF_H
#define FTTF_H

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winioctl.h>
#include <shlobj.h>

#define IDC_DRIVE_COMBO 1001
#define IDC_FORMAT_BUTTON 1002
#define IDC_LOG_EDIT 1003
#define IDC_REFRESH_BUTTON 1004

#define WM_FORMAT_COMPLETE (WM_USER + 1)
#define WM_FORMAT_ERROR (WM_USER + 2)
#define WM_LOG_MESSAGE (WM_USER + 3)

#define MAX_LOG_SIZE 8192

typedef struct {
    WCHAR letter;
    WCHAR label[256];
    ULONGLONG totalSize;
    ULONGLONG freeSize;
} DriveInfo;

typedef struct {
    HWND hwnd;
    HWND hCombo;
    HWND hFormatButton;
    HWND hLogEdit;
    HWND hRefreshButton;
    WCHAR selectedDrive;
    HFONT hFont;
} AppContext;

typedef struct {
    DWORD sectorsPerCluster;
    DWORD bytesPerSector;
    DWORD reservedSectors;
    BYTE numberOfFATs;
    DWORD sectorsPerFAT;
    DWORD totalSectors;
    DWORD rootDirFirstCluster;
} FAT32Params;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
HWND CreateMainWindow(HINSTANCE hInstance);
void CreateControls(HWND hwnd, AppContext* ctx);
void RefreshDriveList(AppContext* ctx);
void OnFormatClicked(AppContext* ctx);
void ResizeControls(HWND hwnd, AppContext* ctx);

int EnumerateDrives(DriveInfo** drives);
void FreeDrives(DriveInfo* drives);

BOOL FormatDrive(WCHAR driveLetter, HWND hwnd);
BOOL CalculateFAT32Parameters(ULONGLONG diskSize, DWORD bytesPerSector, FAT32Params* params);
BOOL WriteFAT32Structures(HANDLE hDrive, FAT32Params* params, HWND hwnd);
BOOL LockVolume(HANDLE hDrive);
BOOL UnmountVolume(HANDLE hDrive);
BOOL DismountVolume(HANDLE hDrive);

void LogMessage(HWND hwnd, const WCHAR* format, ...);
void AppendLog(HWND hEdit, const WCHAR* text);

DWORD WINAPI FormatThreadProc(LPVOID lpParam);

#endif
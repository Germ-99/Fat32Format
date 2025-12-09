#include "../include/fttf.h"

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    AppContext* ctx = (AppContext*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);

    switch (uMsg) {
        case WM_CREATE: {
            ctx = (AppContext*)malloc(sizeof(AppContext));
            if (!ctx) {
                return -1;
            }
            memset(ctx, 0, sizeof(AppContext));
            ctx->hwnd = hwnd;
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)ctx);
            
            ctx->hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            
            CreateControls(hwnd, ctx);
            RefreshDriveList(ctx);
            return 0;
        }

        case WM_SIZE:
            if (ctx) {
                ResizeControls(hwnd, ctx);
            }
            return 0;

        case WM_COMMAND: {
            if (HIWORD(wParam) == BN_CLICKED) {
                if (LOWORD(wParam) == IDC_FORMAT_BUTTON) {
                    OnFormatClicked(ctx);
                } else if (LOWORD(wParam) == IDC_REFRESH_BUTTON) {
                    RefreshDriveList(ctx);
                }
            }
            return 0;
        }

        case WM_FORMAT_COMPLETE:
            LogMessage(ctx->hwnd, L"Format completed successfully!\r\n");
            EnableWindow(ctx->hFormatButton, TRUE);
            EnableWindow(ctx->hCombo, TRUE);
            EnableWindow(ctx->hRefreshButton, TRUE);
            MessageBoxW(hwnd, L"Format completed successfully!", L"Success", MB_ICONINFORMATION);
            return 0;

        case WM_FORMAT_ERROR: {
            WCHAR* errorMsg = (WCHAR*)lParam;
            LogMessage(ctx->hwnd, L"Format failed: %s\r\n", errorMsg);
            EnableWindow(ctx->hFormatButton, TRUE);
            EnableWindow(ctx->hCombo, TRUE);
            EnableWindow(ctx->hRefreshButton, TRUE);
            MessageBoxW(hwnd, errorMsg, L"Error", MB_ICONERROR);
            free(errorMsg);
            return 0;
        }

        case WM_LOG_MESSAGE: {
            WCHAR* msg = (WCHAR*)lParam;
            AppendLog(ctx->hLogEdit, msg);
            free(msg);
            return 0;
        }

        case WM_DESTROY:
            if (ctx) {
                if (ctx->hFont) {
                    DeleteObject(ctx->hFont);
                }
                free(ctx);
            }
            PostQuitMessage(0);
            return 0;
    }

    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

HWND CreateMainWindow(HINSTANCE hInstance) {
    const WCHAR CLASS_NAME[] = L"Fat32FormatWindowClass";

    WNDCLASSW wc = {0};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc)) {
        return NULL;
    }

    HWND hwnd = CreateWindowExW(
        0,
        CLASS_NAME,
        L"FAT32 Format Tool",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 600, 500,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    return hwnd;
}

void CreateControls(HWND hwnd, AppContext* ctx) {
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hwnd, GWLP_HINSTANCE);

    CreateWindowW(L"STATIC", L"Select USB Drive:",
        WS_VISIBLE | WS_CHILD,
        20, 20, 150, 25,
        hwnd, NULL, hInstance, NULL);

    ctx->hCombo = CreateWindowW(L"COMBOBOX", NULL,
        WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_VSCROLL,
        180, 18, 250, 200,
        hwnd, (HMENU)IDC_DRIVE_COMBO, hInstance, NULL);

    ctx->hRefreshButton = CreateWindowW(L"BUTTON", L"Refresh",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        440, 18, 80, 25,
        hwnd, (HMENU)IDC_REFRESH_BUTTON, hInstance, NULL);

    ctx->hFormatButton = CreateWindowW(L"BUTTON", L"Format as FAT32",
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON,
        20, 60, 500, 35,
        hwnd, (HMENU)IDC_FORMAT_BUTTON, hInstance, NULL);

    CreateWindowW(L"STATIC", L"Activity Log:",
        WS_VISIBLE | WS_CHILD,
        20, 110, 150, 25,
        hwnd, NULL, hInstance, NULL);

    ctx->hLogEdit = CreateWindowExW(
        WS_EX_CLIENTEDGE,
        L"EDIT", L"",
        WS_VISIBLE | WS_CHILD | WS_VSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
        20, 140, 540, 250,
        hwnd, (HMENU)IDC_LOG_EDIT, hInstance, NULL);

    CreateWindowW(L"STATIC", L"Software written by Bryson Kelly. Source code available at github.com/Germ-99/Fat32Format",
        WS_VISIBLE | WS_CHILD | SS_CENTER,
        20, 400, 540, 25,
        hwnd, NULL, hInstance, NULL);

    if (ctx->hFont) {
        SendMessageW(ctx->hCombo, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);
        SendMessageW(ctx->hFormatButton, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);
        SendMessageW(ctx->hLogEdit, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);
        SendMessageW(ctx->hRefreshButton, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);
    }
}

void RefreshDriveList(AppContext* ctx) {
    SendMessageW(ctx->hCombo, CB_RESETCONTENT, 0, 0);

    DriveInfo* drives;
    int count = EnumerateDrives(&drives);

    for (int i = 0; i < count; i++) {
        WCHAR displayText[512];
        double sizeGB = (double)drives[i].totalSize / (1024.0 * 1024.0 * 1024.0);
        
        if (wcslen(drives[i].label) > 0) {
            swprintf(displayText, 512, L"%c: - %s (%.2f GB)", 
                drives[i].letter, drives[i].label, sizeGB);
        } else {
            swprintf(displayText, 512, L"%c: - (%.2f GB)", 
                drives[i].letter, sizeGB);
        }

        int idx = (int)SendMessageW(ctx->hCombo, CB_ADDSTRING, 0, (LPARAM)displayText);
        SendMessageW(ctx->hCombo, CB_SETITEMDATA, idx, (LPARAM)drives[i].letter);
    }

    if (count > 0) {
        SendMessageW(ctx->hCombo, CB_SETCURSEL, 0, 0);
    }

    FreeDrives(drives);

    LogMessage(ctx->hwnd, L"Found %d removable drive(s)\r\n", count);
}

void OnFormatClicked(AppContext* ctx) {
    int idx = (int)SendMessageW(ctx->hCombo, CB_GETCURSEL, 0, 0);
    if (idx == CB_ERR) {
        MessageBoxW(ctx->hwnd, L"Please select a drive", L"Error", MB_ICONERROR);
        return;
    }

    WCHAR driveLetter = (WCHAR)SendMessageW(ctx->hCombo, CB_GETITEMDATA, idx, 0);

    WCHAR msg[256];
    swprintf(msg, 256, L"WARNING: All data on drive %c: will be lost!\n\nAre you sure you want to format this drive?", 
        driveLetter);
    
    int result = MessageBoxW(ctx->hwnd, msg, L"Confirm Format", MB_ICONWARNING | MB_YESNO | MB_DEFBUTTON2);
    if (result != IDYES) {
        return;
    }

    EnableWindow(ctx->hFormatButton, FALSE);
    EnableWindow(ctx->hCombo, FALSE);
    EnableWindow(ctx->hRefreshButton, FALSE);

    ctx->selectedDrive = driveLetter;

    DWORD threadId;
    HANDLE hThread = CreateThread(NULL, 0, FormatThreadProc, ctx, 0, &threadId);
    if (hThread) {
        CloseHandle(hThread);
    }
}

void ResizeControls(HWND hwnd, AppContext* ctx) {
    RECT rect;
    GetClientRect(hwnd, &rect);

    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;

    SetWindowPos(ctx->hLogEdit, NULL, 20, 140, width - 40, height - 200, SWP_NOZORDER);
}

void LogMessage(HWND hwnd, const WCHAR* format, ...) {
    WCHAR buffer[1024];
    va_list args;
    va_start(args, format);
    vswprintf(buffer, 1024, format, args);
    va_end(args);

    WCHAR* msg = _wcsdup(buffer);
    PostMessageW(hwnd, WM_LOG_MESSAGE, 0, (LPARAM)msg);
}

void AppendLog(HWND hEdit, const WCHAR* text) {
    int len = GetWindowTextLengthW(hEdit);
    
    if (len > MAX_LOG_SIZE) {
        SetWindowTextW(hEdit, L"");
        len = 0;
    }

    SendMessageW(hEdit, EM_SETSEL, len, len);
    SendMessageW(hEdit, EM_REPLACESEL, FALSE, (LPARAM)text);
    SendMessageW(hEdit, EM_SCROLLCARET, 0, 0);
}

DWORD WINAPI FormatThreadProc(LPVOID lpParam) {
    AppContext* ctx = (AppContext*)lpParam;
    
    BOOL success = FormatDrive(ctx->selectedDrive, ctx->hwnd);
    
    if (success) {
        PostMessageW(ctx->hwnd, WM_FORMAT_COMPLETE, 0, 0);
    } else {
        WCHAR* errorMsg = (WCHAR*)malloc(256 * sizeof(WCHAR));
        wcscpy_s(errorMsg, 256, L"Format operation failed");
        PostMessageW(ctx->hwnd, WM_FORMAT_ERROR, 0, (LPARAM)errorMsg);
    }

    return 0;
}
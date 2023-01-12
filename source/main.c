#include <stdlib.h>
#include "windows.h"

void utf8_to_gb(const char* src, char* dst, int len) {
    int ret = 0;
    WCHAR* strA;
    int i= MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
    if (i <= 0) {
        MessageBox(0, "Failed", "Failed", MB_OK);
        return;
    }
    strA = (WCHAR*)malloc(i * 2);
    MultiByteToWideChar(CP_UTF8, 0, src, -1, strA, i);
    i = WideCharToMultiByte(CP_ACP, 0, strA, -1, NULL, 0, NULL, NULL);
    if (len >= i) {
        ret = WideCharToMultiByte(CP_ACP, 0, strA, -1, dst, i, NULL, NULL);
        dst[i] = 0;
    }
    if (ret <= 0) {
        free(strA);
        return;
    }

    free( strA );
}

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
    char *utf8Title = "原神";
    int len = sizeof("原神");
    char *gbkTitle = (char *)malloc(len);

    utf8_to_gb(utf8Title, gbkTitle, len);
    
    HWND windowHWND = FindWindow(0, gbkTitle);
    if (!windowHWND) {
        MessageBox(windowHWND, "FindWindow has failed", "Failed", MB_OK);
        return -1;
    }

    RECT range;
    if (!GetWindowRect(windowHWND, &range)) {
        MessageBox(windowHWND, "GetWindowRect has failed", "Failed", MB_OK);
        return -1;
    }

    HDC windowHandle = GetWindowDC(windowHWND);
    HDC saveHandle = CreateCompatibleDC(windowHandle);
    if (!saveHandle) {
        MessageBox(windowHWND, "CreateCompatibleDC has failed", "Failed", MB_OK);
        DeleteObject(saveHandle);
        ReleaseDC(windowHWND, windowHandle);
        return -1;
    }

    int nWidth = range.right - range.left;
    int nHeight = range.bottom - range.top;
    HBITMAP bitmapHandle = CreateCompatibleBitmap(windowHandle, nWidth, nHeight);
    if (!bitmapHandle) {
        MessageBox(windowHWND, "CreateBitmap has failed", "Failed", MB_OK);
        DeleteObject(bitmapHandle);
        DeleteObject(saveHandle);
        ReleaseDC(windowHWND, windowHandle);
        return -1;
    }

    HGDIOBJ selectBitmap = SelectObject(saveHandle, bitmapHandle);
    if (!selectBitmap || selectBitmap == HGDI_ERROR) {
        MessageBox(windowHWND, "SelectObject has failed", "Failed", MB_OK);
        DeleteObject(bitmapHandle);
        DeleteObject(saveHandle);
        ReleaseDC(windowHWND, windowHandle);
        return -1;
    }

    if (!BitBlt(saveHandle, 0, 0, nWidth, nHeight, windowHandle, 0, 0, SRCCOPY)) {
        MessageBox(windowHWND, "BitBlt has failed", "Failed", MB_OK);
        DeleteObject(selectBitmap);
        DeleteObject(bitmapHandle);
        DeleteObject(saveHandle);
        ReleaseDC(windowHWND, windowHandle);
        return -1;
    }

    BITMAP bitmap;
    GetObject(bitmapHandle, sizeof(BITMAP), &bitmap);

    BITMAPFILEHEADER   bmfHeader;
    BITMAPINFOHEADER   bi;

    bi.biSize = sizeof(BITMAPINFOHEADER);
    bi.biWidth = bitmap.bmWidth;
    bi.biHeight = bitmap.bmHeight;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;
    bi.biSizeImage = 0;
    bi.biXPelsPerMeter = 0;
    bi.biYPelsPerMeter = 0;
    bi.biClrUsed = 0;
    bi.biClrImportant = 0;

    DWORD dwBmpSize = ((bitmap.bmWidth * bi.biBitCount + 31) / 32) * 4 * bitmap.bmHeight;

    HANDLE hDIB = hDIB = GlobalAlloc(GHND, dwBmpSize);
    char *lpbitmap = (char *)GlobalLock(hDIB);

    GetDIBits(windowHandle, bitmapHandle, 0, (UINT)bitmap.bmHeight, lpbitmap, (BITMAPINFO *)&bi, DIB_RGB_COLORS);

    HANDLE hFile = CreateFile("test.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    
    DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

    bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);
    bmfHeader.bfSize = dwSizeofDIB;
    bmfHeader.bfType = 0x4D42;

    DWORD dwBytesWritten = 0;
    WriteFile(hFile, (LPSTR)&bmfHeader, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)&bi, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
    WriteFile(hFile, (LPSTR)lpbitmap, dwBmpSize, &dwBytesWritten, NULL);

    GlobalUnlock(hDIB);
    GlobalFree(hDIB);
    CloseHandle(hFile);
    DeleteObject(bitmapHandle);
    DeleteObject(selectBitmap);
    DeleteObject(bitmapHandle);
    DeleteObject(saveHandle);
    ReleaseDC(windowHWND, windowHandle);
    
    return 0;
}
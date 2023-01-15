#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

// This function is from https://github.com/lytsing/gbk-utf8/blob/master/utf8.c
void utf8_to_gb(const char* src, char* dst, int len) {
	int ret = 0;
	WCHAR* strA;
	int i= MultiByteToWideChar(CP_UTF8, 0, src, -1, NULL, 0);
	if (i <= 0) {
		printf("ERROR.\n");
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

// Some code is from https://learn.microsoft.com/en-us/windows/win32/gdi/capturing-an-image
int main(int argc, char **argv) {
	printf("Input the name of window you want to get:\n");
	char *title = (char *)malloc(1 << 20);
	gets(title);

	int len = 0;
	for (char *i = title; *i; i++) len++;

	if (*title >> 7) {
		char *gbkTitle = (char *)malloc(len + 1);
		utf8_to_gb(title, gbkTitle, len);
		char *temp = title;
		title = gbkTitle;
		free(temp);
	}

	HWND windowHWND = FindWindow(0, title);
	free(title);
	if (!windowHWND) {
		printf("FindWindow has failed\n");
		return -1;
	}

	RECT range;
	GetWindowRect(windowHWND, &range);

	HDC windowHandle = GetWindowDC(windowHWND);
	HDC saveHandle = CreateCompatibleDC(windowHandle);

	int nWidth = range.right - range.left;
	int nHeight = range.bottom - range.top;
	HBITMAP bitmapHandle = CreateCompatibleBitmap(windowHandle, nWidth, nHeight);

	HGDIOBJ selectBitmap = SelectObject(saveHandle, bitmapHandle);

	if (!BitBlt(saveHandle, 0, 0, nWidth, nHeight, windowHandle, 0, 0, SRCCOPY)) {
		printf("BitBlt has failed");
		goto done;
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

	HANDLE hFile = CreateFile("output.bmp", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	
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
done:
	DeleteObject(selectBitmap);
	DeleteObject(bitmapHandle);
	DeleteObject(saveHandle);
	ReleaseDC(windowHWND, windowHandle);
	
	return 0;
}

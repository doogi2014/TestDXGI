#include "BufferToBmp.h"

bool WriteBmpFile(LPCTSTR sFileName, void* pBits, int cx, int cy, int bitcount)
{
	/*
	* BMP文件说明：
	* 1 - BITMAPFILEHEADE：bfSize    = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 0 + sizeof(RGB)
	*                      bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 0
	* 2 - BITMAPINFO
	*         BITMAPINFOHEADE：biSizeImage = 0
	*         RGBQUAD * 256
	* 3 -  RGB：width*height*bits/8 = (width*bits+31) / 32 * 4 * height
	*/
	HANDLE hFile = CreateFile(sFileName,
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile)
	{
		DWORD dwBmpSize = cx * bitcount / 8 * cy;
		DWORD dwFileSize = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwBmpSize; //14 + 40 + n

		BITMAPFILEHEADER bfh;
		memset(&bfh, 0, sizeof(BITMAPFILEHEADER));
		bfh.bfSize = sizeof(BITMAPFILEHEADER);
		bfh.bfType = 0x4D42; //BM
		bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + 0;

		BITMAPINFOHEADER bih;
		memset(&bih, 0, sizeof(BITMAPINFOHEADER));
		bih.biSize = sizeof(BITMAPINFOHEADER);
		bih.biWidth = cx;
		bih.biHeight = cy;
		bih.biPlanes = 1;
		bih.biBitCount = bitcount;
		bih.biCompression = BI_RGB;
		bih.biSizeImage = 0;
		bih.biXPelsPerMeter = 0;
		bih.biYPelsPerMeter = 0;
		bih.biClrUsed = 0;
		bih.biClrImportant = 0;

		DWORD dwBytesWritten = 0;
		WriteFile(hFile, &bfh, sizeof(BITMAPFILEHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, &bih, sizeof(BITMAPINFOHEADER), &dwBytesWritten, NULL);
		WriteFile(hFile, pBits, dwBmpSize, &dwBytesWritten, NULL);//? pBits : buffer
		// 存储格式是BGRA  A=不透明度
		// pBits是为了用内部结构，写外部数据
		CloseHandle(hFile);
		return true;
	}
	return false;
}
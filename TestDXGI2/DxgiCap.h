#pragma once

#include <stdio.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "BufferToBmp.h"



struct dp_rect_t
{
	RECT   rc;             ///发生变化的矩形框
	/////
	char* line_buffer;    ///矩形框数据起始地址
	int    line_bytes;     ///每行（矩形框width对应）的数据长度
	int    line_nextpos;   ///从0开始，第N行的数据地址: line_buffer + N*line_nextpos 。
	int    line_count;     ///等于矩形框高度 height
};

struct dp_frame_t
{
	int        cx;          ///屏幕宽度
	int        cy;          ///屏幕高度
	int        line_bytes;  ///每个扫描行的实际数据长度
	int        line_stride; ///每个扫描行的4字节对齐的数据长度
	int        bitcount;    ///8.16.24.32 位深度, 8位是256调色板； 16位是555格式的图像

	int        length;      ///屏幕数据长度 line_stride*cy
	char* buffer;      ///屏幕数据
	/////
	int        rc_count;    ///变化区域个数
	dp_rect_t* rc_array;    ///变化区域

	///
	void* param;   ///
};

typedef int(*DP_FRAME) (dp_frame_t* frame);

struct zh_t {
	bool             quit;
	LONG             sleep_msec;
	HANDLE           hThread;
	HANDLE           hEvt;
	DP_FRAME         frame;
	void*            param;
};



BOOL Init();
BOOL AttatchToThread(VOID);
BOOL QueryFrame();
void dp_close();
void dp_create();

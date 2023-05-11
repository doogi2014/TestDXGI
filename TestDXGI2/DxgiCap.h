#pragma once

#include <stdio.h>
#include <d3d11.h>
#include <dxgi1_2.h>
#include "BufferToBmp.h"



struct dp_rect_t
{
	RECT   rc;             ///�����仯�ľ��ο�
	/////
	char* line_buffer;    ///���ο�������ʼ��ַ
	int    line_bytes;     ///ÿ�У����ο�width��Ӧ�������ݳ���
	int    line_nextpos;   ///��0��ʼ����N�е����ݵ�ַ: line_buffer + N*line_nextpos ��
	int    line_count;     ///���ھ��ο�߶� height
};

struct dp_frame_t
{
	int        cx;          ///��Ļ���
	int        cy;          ///��Ļ�߶�
	int        line_bytes;  ///ÿ��ɨ���е�ʵ�����ݳ���
	int        line_stride; ///ÿ��ɨ���е�4�ֽڶ�������ݳ���
	int        bitcount;    ///8.16.24.32 λ���, 8λ��256��ɫ�壻 16λ��555��ʽ��ͼ��

	int        length;      ///��Ļ���ݳ��� line_stride*cy
	char* buffer;      ///��Ļ����
	/////
	int        rc_count;    ///�仯�������
	dp_rect_t* rc_array;    ///�仯����

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

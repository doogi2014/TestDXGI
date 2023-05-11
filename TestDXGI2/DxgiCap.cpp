#include "DxgiCap.h"
#include "web_stream.h"
#include <time.h>

#define RESET_OBJECT(a) if(a){ (a)->Release(); (a)=NULL; }

//windows抓屏(截屏)实现方法
//https://blog.csdn.net/shenyi0106/article/details/56273735

ID3D11Device*           m_hDevice;
ID3D11DeviceContext*    m_hContext;
IDXGIOutputDuplication* m_hDeskDupl;
DXGI_OUTPUT_DESC        m_dxgiOutDesc;
zh_t* dp = NULL;


BOOL Init()
{
	HRESULT hr = S_OK;

	// Driver types supported
	D3D_DRIVER_TYPE DriverTypes[] =
	{
		D3D_DRIVER_TYPE_HARDWARE,
		D3D_DRIVER_TYPE_WARP,
		D3D_DRIVER_TYPE_REFERENCE,
	};
	UINT NumDriverTypes = ARRAYSIZE(DriverTypes);

	// Feature levels supported
	D3D_FEATURE_LEVEL FeatureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_1
	};
	UINT NumFeatureLevels = ARRAYSIZE(FeatureLevels);

	D3D_FEATURE_LEVEL FeatureLevel;

	//
	// Create D3D device
	//
	for (UINT DriverTypeIndex = 0; DriverTypeIndex < NumDriverTypes; ++DriverTypeIndex)
	{
		hr = D3D11CreateDevice(NULL, DriverTypes[DriverTypeIndex], NULL, 0, FeatureLevels, NumFeatureLevels, D3D11_SDK_VERSION, &m_hDevice, &FeatureLevel, &m_hContext);
		if (SUCCEEDED(hr))
		{
			printf("***  -- %d %d  \n", DriverTypeIndex, FeatureLevel);
			break;
		}
	}
	if (FAILED(hr))
	{
		return FALSE;
	}

	//
	// Get DXGI device
	//
	IDXGIDevice* hDxgiDevice = NULL;
	hr = m_hDevice->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&hDxgiDevice));
	if (FAILED(hr))
	{
		return FALSE;
	}

	//
	// Get DXGI adapter
	//
	IDXGIAdapter* hDxgiAdapter = NULL;
	hr = hDxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&hDxgiAdapter));
	RESET_OBJECT(hDxgiDevice);

	if (FAILED(hr))
	{
		return FALSE;
	}

	//
	// Get output
	//
	// 如果有多个桌面的话，指定抓取哪个桌面
	INT nOutput = 0;
	IDXGIOutput* hDxgiOutput = NULL;
	hr = hDxgiAdapter->EnumOutputs(nOutput, &hDxgiOutput);
	RESET_OBJECT(hDxgiAdapter);
	if (FAILED(hr))
	{
		return FALSE;
	}

	//
	// get output description struct
	//
	hDxgiOutput->GetDesc(&m_dxgiOutDesc);

	//
	// QI for Output 1
	//
	IDXGIOutput1* hDxgiOutput1 = NULL;
	hr = hDxgiOutput->QueryInterface(__uuidof(hDxgiOutput1), reinterpret_cast<void**>(&hDxgiOutput1));
	RESET_OBJECT(hDxgiOutput);
	if (FAILED(hr))
	{
		return FALSE;
	}

	//
	// Create desktop duplication
	//
	hr = hDxgiOutput1->DuplicateOutput(m_hDevice, &m_hDeskDupl);
	RESET_OBJECT(hDxgiOutput1);
	if (FAILED(hr))
	{
		return FALSE;
	}

	return TRUE;
}

BOOL AttatchToThread(VOID)
{
	HDESK hCurrentDesktop = OpenInputDesktop(0, FALSE, GENERIC_ALL);
	if (!hCurrentDesktop)
	{
		return FALSE;
	}

	// Attach desktop to this thread
	BOOL bDesktopAttached = SetThreadDesktop(hCurrentDesktop);
	CloseDesktop(hCurrentDesktop);
	hCurrentDesktop = NULL;


	return bDesktopAttached;
}

void AddMouseToFrame(BYTE* bufFrame, int fw, int fh, POINT pos, BYTE* bufMouse, int mw, int mh, int type)
{
	// 当前未处理PtrInfo->ShapeInfo.Type等于1的情况
	// https://docs.microsoft.com/en-us/windows/win32/api/dxgi1_2/ne-dxgi1_2-dxgi_outdupl_pointer_shape_type
	// 关于type==1参考https://blog.csdn.net/peilinok/article/details/106489019

	if (type == 1)
	{
		//注意一定要转换指针类型，四字节和一字节的区别
		unsigned int* myBufFrame = (unsigned int*)bufFrame;
		int cursor_height = mh / 2;
		int cursor_width = mw;
		int shapePitch = 4;
		int top = pos.y;
		int left = pos.x;
		for (int row = 0; row < cursor_height; row++) {
			BYTE MASK = 0x80;
			for (int col = 0; col < cursor_width; col++) {
				// Get masks using appropriate offsets
				BYTE AndMask = bufMouse[(col / 8) + (row * (shapePitch))] & MASK;
				BYTE XorMask = bufMouse[(col / 8) + ((row + cursor_height) * (shapePitch))] & MASK;
				UINT AndMask32 = (AndMask) ? 0xFFFFFFFF : 0xFF000000;
				UINT XorMask32 = (XorMask) ? 0x00FFFFFF : 0x00000000;

				int index = (abs(top) + row) * fw + abs(left) + col;
				// Set new pixel
				// 注意判断索引越界
				if (index < fw * fh)
					myBufFrame[index] = (myBufFrame[index] & AndMask32) ^ XorMask32;

				// Adjust mask
				if (MASK == 0x01)
				{
					MASK = 0x80;
				}
				else
				{
					MASK = MASK >> 1;
				}
			}
		}
	}
	else
	{
		int bitWidth = 4;
		int indexMax = fw * fh * bitWidth;

		for (int i = 0; i < mh; i++) {
			for (int j = 0; j < mw; j++) {
				int indexMouse = j * bitWidth + i * mw * bitWidth;
				int indexFrame = (pos.x + j) * bitWidth + (pos.y + i) * fw * bitWidth;

				// 当鼠标移动到底部时，会出现数组越界，所以要判断indexMax
				// 只处理【透明度】不等于0的像素点
				if (indexFrame < indexMax && bufMouse[indexMouse + 3] != 0) {
					bufFrame[indexFrame + 0] = bufMouse[indexMouse + 0];
					bufFrame[indexFrame + 1] = bufMouse[indexMouse + 1];
					bufFrame[indexFrame + 2] = bufMouse[indexMouse + 2];
				}
			}
		}
	}
}

ID3D11Texture2D* hNewDesktopImage = NULL;
unsigned char* bufMouse = NULL;
DXGI_OUTDUPL_POINTER_SHAPE_INFO ShapeInfo;
POINT mousePoint;

bool showSome(DXGI_OUTDUPL_FRAME_INFO FrameInfo)
{
	if (hNewDesktopImage == NULL)
		return false;

	//
	// create staging buffer for map bits
	//
	IDXGISurface* hStagingSurf = NULL;
	HRESULT hr = hNewDesktopImage->QueryInterface(__uuidof(IDXGISurface), (void**)(&hStagingSurf));
	//RESET_OBJECT(hNewDesktopImage);
	if (FAILED(hr))
	{
		return FALSE;
	}

	//
	// copy bits to user space
	//
	DXGI_MAPPED_RECT mappedRect;
	hr = hStagingSurf->Map(&mappedRect, DXGI_MAP_READ);
	if (SUCCEEDED(hr))
	{
		//nImgSize = GetWidth() * GetHeight() * 3;
		//PrepareBGR24From32(mappedRect.pBits, (BYTE*)pImgData, m_dxgiOutDesc.DesktopCoordinates);
		DXGI_SURFACE_DESC desc;
		memset(&desc, 0, sizeof(DXGI_SURFACE_DESC));
		hStagingSurf->GetDesc(&desc);
		long x = m_dxgiOutDesc.DesktopCoordinates.right - m_dxgiOutDesc.DesktopCoordinates.left;
		long y = m_dxgiOutDesc.DesktopCoordinates.bottom - m_dxgiOutDesc.DesktopCoordinates.top;
		//WriteBmpFile(L"d:\\1.bmp", mappedRect.pBits, x, y, 32);
		//printf("***  -- %d %d %d %d \n", x,y, m_dxgiOutDesc.DesktopCoordinates.top,m_dxgiOutDesc.DesktopCoordinates.left);
		//printf("***  -- %x %d  \n", mappedRect.pBits, mappedRect.Pitch);
		printf("***  -- %d %d %d %d\n", desc.Width, desc.Height, FrameInfo.PointerPosition.Visible, FrameInfo.LastMouseUpdateTime.QuadPart != 0);

		// 添加鼠标
		if (FrameInfo.PointerShapeBufferSize > 0)
		{
			unsigned char* bufMouseTmp = (unsigned char*)malloc(FrameInfo.PointerShapeBufferSize);
			UINT BufferSizeRequired;
			
			// 注意，保证调用此句之前没有执行m_hDeskDupl->ReleaseFrame()
			hr = m_hDeskDupl->GetFramePointerShape(FrameInfo.PointerShapeBufferSize, reinterpret_cast<VOID*>(bufMouseTmp), &BufferSizeRequired, &(ShapeInfo));
			if (SUCCEEDED(hr))
			{
				memcpy(bufMouse, bufMouseTmp, BufferSizeRequired);
				printf("PointerPosition ShapeInfo %d %d %d %d %d\n", ShapeInfo.Type, ShapeInfo.Width, ShapeInfo.Height, ShapeInfo.Pitch, BufferSizeRequired);
			}
			free(bufMouseTmp);
		}

		//FrameInfo.PointerPosition.Visible && FrameInfo.LastMouseUpdateTime.QuadPart != 0
		//不要添加这两个判断，否则鼠标只在移动时显示 && ShapeInfo.Type ==2
		if (bufMouse != NULL)
		{
			//由于鼠标不动时Position会变成0，所以要使用全局变量
			//判断visible为真时，也就是鼠标移动时，才对全局变量赋值
			if (FrameInfo.PointerPosition.Visible)
			{
				mousePoint.x = FrameInfo.PointerPosition.Position.x;
				mousePoint.y = FrameInfo.PointerPosition.Position.y;
			}

			printf("PointerPosition %d %d %d %d\n", mousePoint.x, mousePoint.y, ShapeInfo.Width, ShapeInfo.Height);
			AddMouseToFrame(mappedRect.pBits, desc.Width, desc.Height, mousePoint, bufMouse, ShapeInfo.Width, ShapeInfo.Height, ShapeInfo.Type);
		}

		//tag++;
		//if (tag > 100)
		//{
		//	struct tm local;
		//	time_t t;
		//	t = time(NULL);
		//	//local = localtime_s(&t);
		//	localtime_s(&local,&t);

		//	char tmpbuf[128];
		//	strftime(tmpbuf, 128, "%H_%M_%S", &local);

		//	char hdr[256];
		//	sprintf_s(hdr, "%s -- %d %d %d %d %d\n", tmpbuf, desc.Width, desc.Height, desc.SampleDesc.Count, desc.SampleDesc.Quality, desc.Format);

		//	char fname[128];
		//	sprintf_s(fname, "d:\\11\\%s.bmp", tmpbuf);

		//	int num = MultiByteToWideChar(0, 0, fname, -1, NULL, 0);
		//	wchar_t* wide = new wchar_t[num];
		//	MultiByteToWideChar(0, 0, fname, -1, wide, num);

		//	WriteBmpFile(wide, mappedRect.pBits, desc.Width, desc.Height, 32);

		//	/* FILE* file = fopen(fname, "a+");
		//	 fwrite(hdr, strlen(hdr), 1, file);
		//	 fclose(file);*/
		//	tag = 0;
		//}


		dp_frame_t frm;
		//frm.rc_array = rc_array;
		//frm.rc_count = 32;
		frm.cx = x;
		frm.cy = y;
		//frm.line_bytes = dp->directx.line_bytes;
		frm.line_stride = x * 4;
		frm.bitcount = 32;
		frm.buffer = (char*)mappedRect.pBits;
		//frm.length = dp->directx.line_stride * dp->directx.cy; ///
		//frm.param = dp->param;

		//dp->frame(&frm); /// callback 
		web_stream* a = (web_stream*)dp->param;
		a->frame(&frm);

		hStagingSurf->Unmap();
	}
	//RESET_OBJECT(hNewDesktopImage);
	RESET_OBJECT(hStagingSurf);
	return true;
}

int tag = 0;
BOOL QueryFrame(zh_t* dp)
{
	//if (!AttatchToThread())
	//{
	//	return FALSE;
	//}

	//printf("----11\r\n");
	IDXGIResource* hDesktopResource = NULL;
	DXGI_OUTDUPL_FRAME_INFO FrameInfo;
	// 使用 AcquireNextFrame() 函数获取当前桌面图像， GetFrameDirtyRects() 用来获取发生了变化的矩形区域
	//m_hDeskDupl->GetFrameDirtyRects
	
	
	HRESULT hr = m_hDeskDupl->AcquireNextFrame(50, &FrameInfo, &hDesktopResource);//INFINITE 500
	if (FAILED(hr))
	{
		long a = 0x887a0026;
		bool b = hr == a;
		
		if (hr == DXGI_ERROR_ACCESS_LOST)
		{
			Init();
			//printf("----22 %lx %d\r\n", hr, b);
		}
		//
		// 在一些win10的系统上,如果桌面没有变化的情况下，
		// 这里会发生超时现象，但是这并不是发生了错误，而是系统优化了刷新动作导致的。
		// 所以，这里没必要返回FALSE，返回不带任何数据的TRUE即可
		// 在此处加showSome调用的目的是，就算桌面没有变化也会发送数据，针对的是h.264的优化
		showSome(FrameInfo);
		return TRUE;
	}
	//printf("----33\r\n");

	if (FrameInfo.PointerShapeBufferSize > 0)
	{
		//printf("----4444444444\r\n");
	}

	//
	// query next frame staging buffer
	//
	ID3D11Texture2D* hAcquiredDesktopImage = NULL;
	hr = hDesktopResource->QueryInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&hAcquiredDesktopImage));
	RESET_OBJECT(hDesktopResource);

	if (FAILED(hr))
	{
		
		return FALSE;
	}
	

	//hr = KeyMutex->AcquireSync(0, 1000);

	//
	// copy old description
	//
	D3D11_TEXTURE2D_DESC frameDescriptor;
	hAcquiredDesktopImage->GetDesc(&frameDescriptor);

	//
	// create a new staging buffer for fill frame image
	//
	//ID3D11Texture2D* hNewDesktopImage = NULL;
	if (hNewDesktopImage != NULL)
		hNewDesktopImage->Release();

	frameDescriptor.Usage = D3D11_USAGE_STAGING;
	frameDescriptor.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	frameDescriptor.BindFlags = 0;
	frameDescriptor.MiscFlags = 0;
	frameDescriptor.MipLevels = 1;
	frameDescriptor.ArraySize = 1;
	frameDescriptor.SampleDesc.Count = 1;
	hr = m_hDevice->CreateTexture2D(&frameDescriptor, NULL, &hNewDesktopImage);
	if (FAILED(hr))
	{
		RESET_OBJECT(hAcquiredDesktopImage);
		m_hDeskDupl->ReleaseFrame();
		return FALSE;
	}

	//
	// copy next staging buffer to new staging buffer
	//
	m_hContext->CopyResource(hNewDesktopImage, hAcquiredDesktopImage);


	RESET_OBJECT(hAcquiredDesktopImage);
	

	showSome(FrameInfo);
	m_hDeskDupl->ReleaseFrame();
	return SUCCEEDED(hr);
}



static DWORD CALLBACK __loop_msg(void* _p){

	::CoInitialize(0);

	zh_t* dp = (zh_t*)_p;

	SetEvent(dp->hEvt); 
	//::CoInitialize(0);
	if (bufMouse == NULL)
		bufMouse = (unsigned char*)malloc(2 * 1024 * 1024);

	while (!dp->quit) {
		DWORD t3 = GetTickCount();
		//printf("***  capture screen %d \n", t3);
		QueryFrame(dp);

		Sleep(dp->sleep_msec);
	}

	if (bufMouse)
	{
		free(bufMouse);
		bufMouse = NULL;
	}

	printf("***  loop over \n");
	::CoUninitialize(); ///
	return 0;
}
void dp_close(){

	if (!dp)
		return;
	dp->quit = true;
	SetEvent(dp->hEvt);
	::WaitForSingleObject(dp->hThread, 8 * 1000);
	::TerminateThread(dp->hThread, 0);

	delete dp;
}

int frame_callback(dp_frame_t* frame)
{
	web_stream* web = (web_stream*)frame->param;
	////
	//if (frame->rc_array && frame->rc_count > 0) {//屏幕有变化
	web->frame(frame);
	//}
	return 0;
}

void dp_create()
{
	web_stream* web = new web_stream;

	web->start("0.0.0.0", 29999); // 8000端口侦听

	
	dp = new zh_t;
	dp->quit = false;
	dp->sleep_msec = 50;
	dp->hEvt = CreateEvent(NULL, TRUE, FALSE, NULL);
	dp->frame = frame_callback;
	dp->param = web;

	DWORD tid;
	dp->hThread = CreateThread(NULL, 10 * 1024 * 1024, __loop_msg, dp, 0, &tid);

	if (!dp->hThread) {
		CloseHandle(dp->hEvt);
		delete dp;
	}
	::SetThreadPriority(dp->hThread, THREAD_PRIORITY_HIGHEST); //提升优先级
	::WaitForSingleObject(dp->hEvt, INFINITE);
	::ResetEvent(dp->hEvt); //
}
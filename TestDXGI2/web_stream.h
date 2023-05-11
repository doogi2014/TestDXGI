
#pragma once

#include <WinSock.h>
#include <vector>
#include <string>
using namespace std;
#include "../vs2015_static/jpeglib.h"
#pragma comment(lib,"../vs2015_static/turbojpeg.lib")

#pragma comment(lib,"ws2_32.lib")
#include "DxgiCap.h"

class web_stream
{
protected:
	bool quit;
	int listenfd;
	vector<int> socks;
	mutable CRITICAL_SECTION cs;
	static DWORD CALLBACK accept_thread(void* _p);
	static DWORD CALLBACK client_thread(void* _p);
	///
	int jpeg_quality;
public:
	web_stream();
	~web_stream();

	////
	int start(const char* strip, int port);
	void set_jpeg_quality(int quality) { jpeg_quality = quality; }
	void frame(struct dp_frame_t* frame);
};

#pragma once

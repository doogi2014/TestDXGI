// TestDXGI2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <conio.h>


#include "DxgiCap.h"
#include "web_stream.h"




int main()
{
    WSADATA d;
    WSAStartup(0x0202, &d);

    Init();
    dp_create();
    

    //void* handle = dp_create();
    //printf("\n\n[ESC] to exit\n\n"); 
    //while (_getch() != 27)
    //    Sleep(1000);
    //dp_destroy(handle);

    printf("\n\n[ESC] to exit\n\n"); 
    while (_getch() != 27)
        Sleep(1000);
    dp_close();


    std::cout << "Hello World!\n";
}









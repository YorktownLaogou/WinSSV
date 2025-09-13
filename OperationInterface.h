#pragma once
#include<windows.h>
#include <cstdio>
#include <d2d1.h>
#include <wchar.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <dwrite.h>
#include "FileDirectory.h"
#include "StorageSpaceVisualization.h"
#include "WindowProcs.h"

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Dwrite.lib")

LRESULT CALLBACK WindowProc_(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

class OperationInterface
{

		struct InitializeWindowStruct
		{
			HINSTANCE hInstance;
			UserData_WindowProc_* pUserData;
		};
	public:
		OperationInterface(HINSTANCE hInstance , ID2D1Factory* d2dFactory,IDWriteFactory* pDWriteFactory)
		{
			// (1 创建事件 等待窗口句柄存在
			HANDLE hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
			
			// (2 创建创就线程
			UserData_WindowProc_* pUserData = new UserData_WindowProc_;
			pUserData->d2dFactory = d2dFactory;
			pUserData->pDWriteFactory = pDWriteFactory;
			InitializeWindowStruct _InitializeWindowStruct;
			_InitializeWindowStruct.hInstance = hInstance;
			_InitializeWindowStruct.pUserData = pUserData;
			CreateThread(NULL ,0 , InitializeWindow, &_InitializeWindowStruct, 0 , NULL);

			// (3 等待事件返回
			WaitForSingleObject(hEvent, INFINITE);
			CloseHandle(hEvent);
		}


	private:


		static DWORD WINAPI InitializeWindow(LPVOID lpParam)
		{
			const wchar_t CLASS_NAME[] = L"SampleWindowClass";

			// (1 注册窗口类
			WNDCLASS wc = {};
			wc.lpfnWndProc = WindowProc_;
			wc.hInstance = ((InitializeWindowStruct*)lpParam)->hInstance;
			wc.lpszClassName = CLASS_NAME;

			RegisterClass(&wc);

			// (2 创建窗口
			HWND hwnd = CreateWindowExW(
				0,
				CLASS_NAME,
				L"空间可视化",
				WS_OVERLAPPEDWINDOW,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				NULL,
				NULL,
				((InitializeWindowStruct*)lpParam)->hInstance,
				((InitializeWindowStruct*)lpParam)->pUserData
			);
			if (hwnd == NULL)return 0;
			ShowWindow(hwnd, 1);

			// (3 消息循环
			MSG msg = {};
			while (GetMessage(&msg, NULL, 0, 0)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}

			exit(0);
			return 0;
		}

};
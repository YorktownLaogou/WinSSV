#pragma once
#include <windows.h>
#include <cstdio>
#include <d2d1.h>
#include <wchar.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <dwrite.h>
#include "FileDirectory.h"
#include "StorageSpaceVisualization.h"


#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Dwrite.lib")

// 预览器页面
struct UserData_WindowProc_
{
	ID2D1BitmapRenderTarget* pBmpRenderTarget;	// 屏幕绘制缓冲区
	ID2D1HwndRenderTarget* pRenderTarget;		// 渲染对象
	ID2D1Factory* d2dFactory;					// D2D工厂
	IDWriteFactory* pDWriteFactory;				// 写字工厂

	FileDirectory* FileDirectory_;
	StorageSpaceVisualization* SSV;
	FileDirectory::FileDirectoryNode* WorkingNode;



	UINT32 Focus;
};
LRESULT CALLBACK WindowProc_(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_SIZE:
		{
			UserData_WindowProc_* pData = reinterpret_cast<UserData_WindowProc_*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			// 老Size
			D2D1_SIZE_F size = pData->pBmpRenderTarget->GetSize();
			float owidth = size.width;
			float oheight = size.height;
			

			// 刷新D2DBitmap大小
			if (pData->pBmpRenderTarget) pData->pBmpRenderTarget->Release();
			UINT width = LOWORD(lParam);
			UINT height = HIWORD(lParam);
			pData->pRenderTarget->CreateCompatibleRenderTarget(D2D1::SizeF(width, height), &pData->pBmpRenderTarget);
			D2D1_SIZE_U newSize = D2D1::SizeU(width, height);
			pData->pRenderTarget->Resize(newSize);

			// 刷新窗口
			if (width < owidth || height < oheight) 
			{
				InvalidateRect(hWnd, NULL, TRUE);
			}


			return 0;
		}
		case WM_CREATE:
		{
			// (1 绑定窗口指针
			CREATESTRUCT* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			UserData_WindowProc_* pData = reinterpret_cast<UserData_WindowProc_*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pData));

			// (2 创建磁盘目录实例
			pData->FileDirectory_ = new FileDirectory();
			pData->FileDirectory_->InitializeDirectory();
			pData->WorkingNode = pData->FileDirectory_->DiskFileDirectory;
			pData->FileDirectory_->SortSublist(pData->WorkingNode);
			pData->SSV = new StorageSpaceVisualization();

			// (3 创建缓冲Mitmap
			D2D1_RENDER_TARGET_PROPERTIES rtProps = D2D1::RenderTargetProperties();
			D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(hWnd, D2D1::SizeU(0, 0));
			pData->d2dFactory->CreateHwndRenderTarget(rtProps, hwndProps, &(pData->pRenderTarget));
			pData->pRenderTarget->CreateCompatibleRenderTarget(&pData->pBmpRenderTarget);

			return 0;
		}
		case WM_PAINT:
		{
			UserData_WindowProc_* pData = reinterpret_cast<UserData_WindowProc_*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			pData->pBmpRenderTarget->BeginDraw();

			// (1 分布部分
			WCHAR** NameList = new WCHAR * [pData->WorkingNode->SubfileNum];
			UINT64* SizeList = new UINT64[pData->WorkingNode->SubfileNum];
			for (int i = 0; i < pData->WorkingNode->SubfileNum; i++)
			{
				SizeList[i] = pData->WorkingNode->SubDirectory[i]->FileSize;
				NameList[i] = pData->WorkingNode->SubDirectory[i]->cFileName;
			}
			pData->SSV->UpdateFileList(SizeList, NameList, pData->WorkingNode->SubfileNum);
			pData->SSV->PaintD2DBitmap(pData->pBmpRenderTarget, pData->pDWriteFactory);
			delete[] NameList;
			delete[] SizeList;
			HRESULT hr =  pData->pBmpRenderTarget->EndDraw();

			ID2D1Bitmap* pBitmap = nullptr;
			hr = pData->pBmpRenderTarget->GetBitmap(&pBitmap);
			pData->pRenderTarget->BeginDraw();
			pData->pRenderTarget->DrawBitmap(pBitmap);
			hr = pData->pRenderTarget->EndDraw();
			pBitmap->Release();

			EndPaint(hWnd, &ps);

			return 0;
		}
		case WM_LBUTTONUP:
		{
			UserData_WindowProc_* pData = reinterpret_cast<UserData_WindowProc_*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			UINT x = LOWORD(lParam);
			UINT y = HIWORD(lParam);
			POINT point = {x,y};
			RECT rect;
			GetClientRect(hWnd, &rect);
			int WndWidth = rect.right - rect.left;
			int WndHeight = rect.bottom - rect.top;

			// (1 响应空间可视化的响应
			RECT VisualizationRect = {0,0,WndWidth ,WndHeight };
			INT hr = pData->SSV->FindRect(point , VisualizationRect);
			if(hr != -1 && pData->WorkingNode->SubDirectory[hr]->SubfileNum != 0)
			{
				pData->WorkingNode = pData->WorkingNode->SubDirectory[hr];
				pData->FileDirectory_->SortSublist( pData->WorkingNode);
				InvalidateRect(hWnd, NULL, TRUE);
				return 0;
			}
			return 0;
		}
		case WM_RBUTTONUP:
		{
			UserData_WindowProc_* pData = reinterpret_cast<UserData_WindowProc_*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
			UINT x = LOWORD(lParam);
			UINT y = HIWORD(lParam);
			POINT point = { x,y };

			if (pData->WorkingNode->PrtDirectory != nullptr)
			{
				pData->WorkingNode = pData->WorkingNode->PrtDirectory;
				InvalidateRect(hWnd, NULL, TRUE);
				return 0;
			}
			return 0;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			return 0;
		}
		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
}
#pragma once
#include<windows.h>
#include <cstdio>
#include <d2d1.h>
#include <wchar.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <dwrite.h>

#pragma comment(lib, "d2d1.lib")
#pragma comment(lib, "windowscodecs.lib")
#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "Dwrite.lib")

class StorageSpaceVisualization
{
	public:
		// 更新文件列表
		DWORD UpdateFileList(UINT64* FileSizeList , WCHAR** FileNAmeList , UINT32 ListSize)
		{
			if(_FileSizeList)delete[] _FileSizeList;
			_FileSizeList = new UINT64[ListSize];
			if (_FileNAmeList)
			{
				for(int i = 0 ; i < _ListSize ; i++)
				{
					delete[] _FileNAmeList[i];
				}
				delete[] _FileNAmeList;
			}
			_FileNAmeList = new WCHAR*[ListSize];

			for(int i = 0 ; i < ListSize ; i++)
			{
				_FileSizeList[i] = FileSizeList[i];
				_FileNAmeList[i] = new WCHAR[wcslen(FileNAmeList[i])+1];
				wmemcpy(_FileNAmeList[i], FileNAmeList[i], wcslen(FileNAmeList[i])+1);
			}
			_ListSize = ListSize;

			CalculateFileSizeLyout(_FileSizeList , _ListSize);

			return 1;
		}





		// 图片保存到文件
		DWORD SaveLayoutTofie(IWICImagingFactory* wic , ID2D1Factory* d2d , IDWriteFactory* pDWriteFactory)
		{
			// 1) 创建 2000x2000 的 WIC 位图
			UINT W = 5000, H = 5000;
			IWICBitmap* wicBitmap;
			wic->CreateBitmap(W, H, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &wicBitmap);

			// 2) 基于 WIC 位图创建 D2D 渲染目标（离屏）
			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				96.0f, 96.0f, 
				D2D1_RENDER_TARGET_USAGE_NONE,
				D2D1_FEATURE_LEVEL_DEFAULT
			);
			ID2D1RenderTarget* rt;
			d2d->CreateWicBitmapRenderTarget(wicBitmap, props, &rt);

			// 3) 创建画笔然后清除背景 绘制
			IDWriteTextFormat* pTextFormat = nullptr;
			pDWriteFactory->CreateTextFormat(
				L"Segoe UI",                // 字体名称
				nullptr,                    // 字体集合
				DWRITE_FONT_WEIGHT_REGULAR, // 字重
				DWRITE_FONT_STYLE_NORMAL,   // 字体样式
				DWRITE_FONT_STRETCH_NORMAL, // 拉伸
				32.0f,                      // 字号
				L"en-us",                   // 本地化
				&pTextFormat
			);
			ID2D1SolidColorBrush* brush;
			rt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f), &brush);
			rt->BeginDraw();
			rt->Clear(D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f));
			for(int i = 0 ; i < _ListSize ; i++)
			{
				D2D1_RECT_F rect ={ _Layout[i].left*W, _Layout[i].top*H, _Layout[i].right*W, _Layout[i].bottom*H};
				rt->DrawRectangle(&rect, brush, 3.0f);
				rt->DrawTextW(
					_FileNAmeList[i],			// 文本
					wcslen(_FileNAmeList[i]),	// 长度
					pTextFormat,				// 文本格式
					rect,						// 绘制区域
					brush						// 画刷
				);
			}
			rt->EndDraw();

			// 4) 创建保存文件的流
			IWICStream* pStream;
			wic->CreateStream(&pStream);
			pStream->InitializeFromFilename(L"D:\\output.png", GENERIC_WRITE);

			// 5) 创建编码器
			IWICBitmapEncoder* pEncoder;
			wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEncoder);
			pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);

			// 6) 创建新帧
			IWICBitmapFrameEncode* pFrameEncode;
			IPropertyBag2* pPropertyBag;
			DWORD hr = pEncoder->CreateNewFrame(&pFrameEncode, &pPropertyBag);
			pFrameEncode->Initialize(nullptr);

			// 7) 设置位图大小和格式
			pFrameEncode->SetSize(W, H);
			WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
			pFrameEncode->SetPixelFormat(&format);

			// 8) 写入位图数据
			pFrameEncode->WriteSource(wicBitmap, nullptr);

			// 9) 提交帧和编码器
			pFrameEncode->Commit();
			pEncoder->Commit();

			// 10) 释放
			pPropertyBag->Release();
			pFrameEncode->Release();
			pEncoder->Release();
			pStream->Release();

			return 0;
		}





		// 图片渲染到D2Dbitmap
		DWORD PaintD2DBitmap(ID2D1BitmapRenderTarget* pRt, IDWriteFactory* pDWriteFactory)
		{
			D2D1_SIZE_U size = pRt->GetPixelSize();
			UINT width = size.width;
			UINT height = size.height;
			
			IDWriteTextFormat* pTextFormat = nullptr;
			pDWriteFactory->CreateTextFormat(
				L"Segoe UI",                // 字体名称
				nullptr,                    // 字体集合
				DWRITE_FONT_WEIGHT_REGULAR, // 字重
				DWRITE_FONT_STYLE_NORMAL,   // 字体样式
				DWRITE_FONT_STRETCH_NORMAL, // 拉伸
				15.0f,                      // 字号
				L"en-us",                   // 本地化
				&pTextFormat
			);
			pTextFormat->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
			ID2D1SolidColorBrush* brush;
			pRt->CreateSolidColorBrush(D2D1::ColorF(1.0f, 0.0f, 0.0f, 1.0f), &brush);
			pRt->Clear(D2D1::ColorF(0.0f, 1.0f, 0.0f, 1.0f));

			for (int i = 0; i < _ListSize; i++)
			{
				D2D1_RECT_F _rect = { _Layout[i].left * width, _Layout[i].top * height, _Layout[i].right * width, _Layout[i].bottom * height };
				pRt->DrawRectangle(&_rect, brush, 2.0f);
				pRt->DrawTextW(
					_FileNAmeList[i],                    // 文本
					wcslen(_FileNAmeList[i]),            // 长度
					pTextFormat,                         // 文本格式
					_rect,                               // 绘制区域
					brush,                               // 画刷
					D2D1_DRAW_TEXT_OPTIONS_CLIP,         // 启用裁剪
					DWRITE_MEASURING_MODE_NATURAL        // 测量模式（可选）
				);

			}
			pTextFormat->Release();
			brush->Release();

			return 1;
		}




	
		// 在范围内找 并返回角标
		DWORD FindRect(POINT point , RECT rect)
		{
			UINT AreaWidth = rect.right - rect.left;
			UINT AreaHeight = rect.bottom - rect.top;
			for(int i = 0 ; i < _ListSize ; i++)
			{
				if
				(
					point.x >= rect.left + _Layout[i].left * AreaWidth &&
					point.x < rect.left + _Layout[i].right * AreaWidth &&
					point.y >= rect.top + _Layout[i].top * AreaHeight &&
					point.y < rect.top + _Layout[i].bottom * AreaHeight
				)
				{
					return i;
				}
			}
			return -1;
		}
	private:
		UINT64* _FileSizeList;	// 文件夹大小列表
		WCHAR**	_FileNAmeList;	// 文件夹名字列表
		D2D1_RECT_F* _Layout;	// 布局数组
		UINT32 _ListSize;		// 列表的大小



		// 输入大小名字列表 输出RECT列表 脚标和名字列表对应 不新排列数组
		// 左上最大 右下角最小 
		// 二分法 传入的应该排序了 小的在右下
		DWORD CalculateFileSizeLyout(UINT64* iFileSizeList, UINT FileNum)
		{
			_Layout = new D2D1_RECT_F[FileNum];
			_Layout[0] = { 0.0f,0.0f,1.0f,1.0f };
			BisectionMethodLayout(iFileSizeList, _Layout, FileNum);

			return 0;
		}
		DWORD BisectionMethodLayout(UINT64* iFileSizeList, D2D1_RECT_F* layout, UINT FileNum)
		{
			if (FileNum <= 1)return 1;

			// 计算比例
			UINT32 LeftHalfPart = FileNum / 2;
			UINT32 RightHalfPart = FileNum - LeftHalfPart;
			UINT64 LeftHalfPartSize = 0, RightHalfPartSize = 0;
			for (int i = 0; i < LeftHalfPart; i++)
			{
				LeftHalfPartSize += iFileSizeList[i];
			}
			for (int i = LeftHalfPart; i < FileNum; i++)
			{
				RightHalfPartSize += iFileSizeList[i];
			}

			// 切长边
			if (layout[0].bottom - layout[0].top < layout[0].right - layout[0].left)
			{
				float result = layout[0].left + (layout[0].right - layout[0].left) * (static_cast<float>(LeftHalfPartSize) / (RightHalfPartSize+ LeftHalfPartSize));
				layout[LeftHalfPart].top = layout[0].top;
				layout[LeftHalfPart].right = layout[0].right;
				layout[LeftHalfPart].bottom = layout[0].bottom;
				layout[0].right = result;
				layout[LeftHalfPart].left = result;
			}
			else
			{
				float result = layout[0].top + (layout[0].bottom - layout[0].top) * (static_cast<float>(LeftHalfPartSize) / (RightHalfPartSize + LeftHalfPartSize));
				layout[LeftHalfPart].right = layout[0].right;
				layout[LeftHalfPart].bottom = layout[0].bottom;
				layout[LeftHalfPart].left = layout[0].left;
				layout[0].bottom = result;
				layout[LeftHalfPart].top = result;
			}

			// 如果大于等于2 则递归
			if (FileNum >= 2)
			{
				BisectionMethodLayout(iFileSizeList, layout, LeftHalfPart);
				BisectionMethodLayout(&iFileSizeList[LeftHalfPart], &layout[LeftHalfPart], RightHalfPart);
			}

			return 1;
		}


		// 输入绘制对象 大小列表 名字列表
		DWORD CreateSpatialDistributionBitmup()
		{



		}
};


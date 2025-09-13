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
		// �����ļ��б�
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





		// ͼƬ���浽�ļ�
		DWORD SaveLayoutTofie(IWICImagingFactory* wic , ID2D1Factory* d2d , IDWriteFactory* pDWriteFactory)
		{
			// 1) ���� 2000x2000 �� WIC λͼ
			UINT W = 5000, H = 5000;
			IWICBitmap* wicBitmap;
			wic->CreateBitmap(W, H, GUID_WICPixelFormat32bppPBGRA, WICBitmapCacheOnLoad, &wicBitmap);

			// 2) ���� WIC λͼ���� D2D ��ȾĿ�꣨������
			D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
				D2D1_RENDER_TARGET_TYPE_DEFAULT,
				D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
				96.0f, 96.0f, 
				D2D1_RENDER_TARGET_USAGE_NONE,
				D2D1_FEATURE_LEVEL_DEFAULT
			);
			ID2D1RenderTarget* rt;
			d2d->CreateWicBitmapRenderTarget(wicBitmap, props, &rt);

			// 3) ��������Ȼ��������� ����
			IDWriteTextFormat* pTextFormat = nullptr;
			pDWriteFactory->CreateTextFormat(
				L"Segoe UI",                // ��������
				nullptr,                    // ���弯��
				DWRITE_FONT_WEIGHT_REGULAR, // ����
				DWRITE_FONT_STYLE_NORMAL,   // ������ʽ
				DWRITE_FONT_STRETCH_NORMAL, // ����
				32.0f,                      // �ֺ�
				L"en-us",                   // ���ػ�
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
					_FileNAmeList[i],			// �ı�
					wcslen(_FileNAmeList[i]),	// ����
					pTextFormat,				// �ı���ʽ
					rect,						// ��������
					brush						// ��ˢ
				);
			}
			rt->EndDraw();

			// 4) ���������ļ�����
			IWICStream* pStream;
			wic->CreateStream(&pStream);
			pStream->InitializeFromFilename(L"D:\\output.png", GENERIC_WRITE);

			// 5) ����������
			IWICBitmapEncoder* pEncoder;
			wic->CreateEncoder(GUID_ContainerFormatPng, nullptr, &pEncoder);
			pEncoder->Initialize(pStream, WICBitmapEncoderNoCache);

			// 6) ������֡
			IWICBitmapFrameEncode* pFrameEncode;
			IPropertyBag2* pPropertyBag;
			DWORD hr = pEncoder->CreateNewFrame(&pFrameEncode, &pPropertyBag);
			pFrameEncode->Initialize(nullptr);

			// 7) ����λͼ��С�͸�ʽ
			pFrameEncode->SetSize(W, H);
			WICPixelFormatGUID format = GUID_WICPixelFormat32bppPBGRA;
			pFrameEncode->SetPixelFormat(&format);

			// 8) д��λͼ����
			pFrameEncode->WriteSource(wicBitmap, nullptr);

			// 9) �ύ֡�ͱ�����
			pFrameEncode->Commit();
			pEncoder->Commit();

			// 10) �ͷ�
			pPropertyBag->Release();
			pFrameEncode->Release();
			pEncoder->Release();
			pStream->Release();

			return 0;
		}





		// ͼƬ��Ⱦ��D2Dbitmap
		DWORD PaintD2DBitmap(ID2D1BitmapRenderTarget* pRt, IDWriteFactory* pDWriteFactory)
		{
			D2D1_SIZE_U size = pRt->GetPixelSize();
			UINT width = size.width;
			UINT height = size.height;
			
			IDWriteTextFormat* pTextFormat = nullptr;
			pDWriteFactory->CreateTextFormat(
				L"Segoe UI",                // ��������
				nullptr,                    // ���弯��
				DWRITE_FONT_WEIGHT_REGULAR, // ����
				DWRITE_FONT_STYLE_NORMAL,   // ������ʽ
				DWRITE_FONT_STRETCH_NORMAL, // ����
				15.0f,                      // �ֺ�
				L"en-us",                   // ���ػ�
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
					_FileNAmeList[i],                    // �ı�
					wcslen(_FileNAmeList[i]),            // ����
					pTextFormat,                         // �ı���ʽ
					_rect,                               // ��������
					brush,                               // ��ˢ
					D2D1_DRAW_TEXT_OPTIONS_CLIP,         // ���òü�
					DWRITE_MEASURING_MODE_NATURAL        // ����ģʽ����ѡ��
				);

			}
			pTextFormat->Release();
			brush->Release();

			return 1;
		}




	
		// �ڷ�Χ���� �����ؽǱ�
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
		UINT64* _FileSizeList;	// �ļ��д�С�б�
		WCHAR**	_FileNAmeList;	// �ļ��������б�
		D2D1_RECT_F* _Layout;	// ��������
		UINT32 _ListSize;		// �б�Ĵ�С



		// �����С�����б� ���RECT�б� �ű�������б��Ӧ ������������
		// ������� ���½���С 
		// ���ַ� �����Ӧ�������� С��������
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

			// �������
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

			// �г���
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

			// ������ڵ���2 ��ݹ�
			if (FileNum >= 2)
			{
				BisectionMethodLayout(iFileSizeList, layout, LeftHalfPart);
				BisectionMethodLayout(&iFileSizeList[LeftHalfPart], &layout[LeftHalfPart], RightHalfPart);
			}

			return 1;
		}


		// ������ƶ��� ��С�б� �����б�
		DWORD CreateSpatialDistributionBitmup()
		{



		}
};


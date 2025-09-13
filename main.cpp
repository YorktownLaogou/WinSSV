#include "FileDirectory.h"
#include "StorageSpaceVisualization.h"
#include "OperationInterface.h"




int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) 
{
	// ��ʼ�� COM
	HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);

	// ���� WIC ����
	IWICImagingFactory* wicFactory;
	hr = CoCreateInstance(
		CLSID_WICImagingFactory,
		nullptr,
		CLSCTX_INPROC_SERVER,
		IID_PPV_ARGS(&wicFactory)
	);

	// ���� Direct2D ����
	ID2D1Factory* d2dFactory;
	D2D1_FACTORY_OPTIONS options = {};
	options.debugLevel = D2D1_DEBUG_LEVEL_INFORMATION; // ��Ϣ����
	hr = D2D1CreateFactory(
		D2D1_FACTORY_TYPE_SINGLE_THREADED,
		__uuidof(ID2D1Factory),
		&options,
		reinterpret_cast<void**>(&d2dFactory)
	);

	// ��ʼ�� DirectWrite ����
	IDWriteFactory* pDWriteFactory = nullptr;
	hr = DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(IDWriteFactory),
		reinterpret_cast<IUnknown**>(&pDWriteFactory)
	);
	



	// ��������
	OperationInterface* OperationInterface_ = new OperationInterface(hInstance , d2dFactory , pDWriteFactory);



	Sleep(1145141919810);
};


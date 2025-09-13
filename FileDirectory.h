#pragma once
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <iostream>


/* 
�ļ�Ŀ¼�� 
�ص��Ǵ���Ŀ¼ָ�� 
	�ڵ������ƴ�С���б�
	��������ROOT�ڵ�û���κζ��� ����DiskFileDirectory ���б���������� û�ø��ڵ�
��Ҫ����
	�����б� 
		�ݹ� (������� �߼����� �ļ����ļ���) ���������Լ�һ��������Ϊ���

*/
class FileDirectory
{
	public:
		// ˽�еĴ�����Ŀ¼�Ľṹ�� ��ʾ�ļ���ʱ��ڵ�������Ϊnull
		class FileDirectoryNode
		{
			friend class FileDirectory;
			public:
				DWORD	Attribute;		// �ļ���Ϣ ��ǩ
				WCHAR*	cFileName;		// �ڵ����� �ļ�����
				UINT64	FileSize;		// ʵ��ʹ�õĿռ�
				UINT32 SubfileNum;		// ��ȷ�������ļ�����

				// �ļ���Ŀ¼�б�͸���ָ��
				FileDirectoryNode** SubDirectory;
				FileDirectoryNode* PrtDirectory;
				

				FileDirectoryNode()
				{
					ArrayLen = 0 ;
					SubfileNum = 0;
					SubDirectory = nullptr;
					PrtDirectory = nullptr;
					FileSize = 0;
					Attribute = NULL;
					cFileName = nullptr;
					FileSize = 0;
				}
				~FileDirectoryNode()
				{
					delete[] cFileName;
					for(int i = 0 ; i < SubfileNum ; i++)
					{
						delete SubDirectory[i];
					}
					delete[] SubDirectory;
				}

			private:
				// �������б���
				UINT32 ArrayLen;	// ��ǰ����ĳ���
				void AddSublistItem(FileDirectoryNode* newItem)
				{
					// �ж��Ƿ񳬳�����
					if (SubfileNum >= ArrayLen)
					{
						// �µ����鳤�� ����������
						UINT newLen = ArrayLen + 10;
						FileDirectoryNode** newArray = new FileDirectoryNode * [newLen];

						// �Ѿɵ����ݸ��Ƶ�������
						if (SubDirectory != nullptr)
						{
							memcpy(newArray, SubDirectory, SubfileNum * sizeof(FileDirectoryNode*));
							delete[] SubDirectory;
						}

						// ����ָ��
						SubDirectory = newArray;
						ArrayLen = newLen;
					}

					// �����Ԫ��
					SubDirectory[SubfileNum] = newItem;
					SubfileNum++;
				}
		};
		class FileDirectoryNode_A : public FileDirectoryNode 
		{
			public:
				UINT64	QuotaSize;		// ����С ������̴�С������С

				FileDirectoryNode_A()
				{
					QuotaSize = 0;
				}
		};




	public:
		FileDirectoryNode* DiskFileDirectory;	// ������̵�Ŀ¼
		UINT32 NodeCounter;						// ������






		// �������б� ������� �������ڵ� 
		DWORD InitializeDirectory()
		{
			// �������ڵ�
			if (DiskFileDirectory) delete DiskFileDirectory;
			DiskFileDirectory = new FileDirectoryNode;

			// ����
			CreatNodes_Volume(NULL,DiskFileDirectory);

			return 1;
		}
		




		// �������б� ��������ֱ��ð��
		DWORD SortSublist(FileDirectoryNode* WorkingNode)
		{
			for(int i = 0 ; i < WorkingNode->SubfileNum - 1; i++)
			{
				for (int j = 0; j < WorkingNode->SubfileNum - i - 1; j++)
				{
					if(WorkingNode->SubDirectory[j]->FileSize < WorkingNode->SubDirectory[j+1]->FileSize)
					{
						FileDirectoryNode* pNodeBuffer = WorkingNode->SubDirectory[j + 1];
						WorkingNode->SubDirectory[j + 1] = WorkingNode->SubDirectory[j];
						WorkingNode->SubDirectory[j] = pNodeBuffer;
					}
				}
			}
			return 1;
		}





	private:

		/* ------------- �����ļ�Ŀ¼���� ------------- */

		// ������̱��� Ϊ���ڵ㴴���ӽڵ㣨�����ڵ��ַ��
		DWORD CreatNodes_Disk		(WCHAR* URL ,FileDirectoryNode* PrtDirectoryNode)
		{
			for(int i = 0 ; i < 32 ; i++)
			{
				// ������̱��
				WCHAR DiskFind[64];
				swprintf(DiskFind, 64, L"\\\\.\\PhysicalDrive%d", i);
				HANDLE hDevice = CreateFileW
				(
					DiskFind,
					GENERIC_READ | GENERIC_WRITE,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					NULL
				);
				if (hDevice == INVALID_HANDLE_VALUE)continue;

				// ������̴����򴴽��ӽڵ�
				FileDirectoryNode_A* pDisk = new FileDirectoryNode_A;
				pDisk->PrtDirectory = PrtDirectoryNode;
				PrtDirectoryNode->AddSublistItem(pDisk);

				// ȡ�ô�������
				DISK_GEOMETRY_EX dgex = { 0 };
				DWORD bytesReturned = 0;
				DeviceIoControl
				(
					hDevice,
					IOCTL_DISK_GET_DRIVE_GEOMETRY_EX,
					NULL,
					0,
					&dgex,
					sizeof(dgex),
					&bytesReturned,
					NULL
				);
				pDisk->FileSize = dgex.DiskSize.QuadPart;
				
				// ȡ������
				STORAGE_PROPERTY_QUERY query = { };
				query.PropertyId = StorageDeviceProperty;
				query.QueryType = PropertyStandardQuery;
				BYTE buffer[1024] = { 0 };
				if (DeviceIoControl(
					hDevice,
					IOCTL_STORAGE_QUERY_PROPERTY,
					&query,
					sizeof(query),
					&buffer,
					sizeof(buffer),
					&bytesReturned,
					NULL))
				{
					STORAGE_DEVICE_DESCRIPTOR* desc = (STORAGE_DEVICE_DESCRIPTOR*)buffer;
					if (desc->ProductIdOffset)
					{
						const char* ansiName = (const char*)buffer + desc->ProductIdOffset;
						int len = MultiByteToWideChar(CP_ACP, 0, ansiName, -1, NULL, 0);
						pDisk->cFileName = new WCHAR[len];
						MultiByteToWideChar(CP_ACP, 0, ansiName, -1, pDisk->cFileName, len);
					}	
				}

				// �ͷ� 
				CloseHandle(hDevice);
			}
			return 0;
		}
		DWORD CreatNodes_Volume		(WCHAR* URL ,FileDirectoryNode* PrtDirectoryNode)
		{
			//�������з־� ���ڵ㻹�Ǹ��ڵ�URl��������һ����ѡ����
			WCHAR volumeName[MAX_PATH];
			HANDLE hFind = FindFirstVolumeW(volumeName, MAX_PATH);
			if (hFind == INVALID_HANDLE_VALUE) return 0;
			do 
			{
				// �򿪾�
				size_t len = wcslen(volumeName);
				if (len > 0 && volumeName[len - 1] == L'\\') volumeName[len - 1] = L'\0';
				HANDLE hVol = CreateFileW(
					volumeName,
					GENERIC_READ,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					OPEN_EXISTING,
					0,
					NULL
				);
				if (hVol != INVALID_HANDLE_VALUE)
				{
					// ��1 �����ӽڵ�
					FileDirectoryNode_A* pVolume = new FileDirectoryNode_A;
					pVolume->PrtDirectory = PrtDirectoryNode;
					PrtDirectoryNode->AddSublistItem(pVolume);
					
					// ��2 ��¼���ص����� �ʹ�С
					WCHAR volumePaths[MAX_PATH] = { 0 };
					DWORD returnLength = 0;
					volumeName[len - 1] = L'\\';
					if (GetVolumePathNamesForVolumeNameW(volumeName, volumePaths, MAX_PATH, &returnLength))
					{
						// ��¼���ص� ֻѡȡ��һ��
						pVolume->cFileName = new WCHAR[wcslen(volumePaths) + 1];
						wmemcpy(pVolume->cFileName, volumePaths, wcslen(volumePaths) + 1);

						// ��ȡ���� / ���� / ʣ��
						ULARGE_INTEGER freeBytesAvailable = { 0 };
						ULARGE_INTEGER totalBytes = { 0 };
						ULARGE_INTEGER totalFreeBytes = { 0 };
						if (GetDiskFreeSpaceExW(volumeName,&freeBytesAvailable,&totalBytes,&totalFreeBytes))
						{
							ULONGLONG used = totalBytes.QuadPart - totalFreeBytes.QuadPart;
							pVolume->QuotaSize = totalBytes.QuadPart;
							if(volumePaths[0] == L'\0')
							{
								pVolume->FileSize = used;
							}
							else
							{
								// �ݹ� �����ļ�
								CreatNodes_Default(volumePaths, pVolume);
							}
						}
						// DWORD ERROR_ = GetLastError();
						Sleep(1);
					}
					// �ͷ�
					CloseHandle(hVol);
				}
			} while (FindNextVolumeW(hFind, volumeName, MAX_PATH));

			return 1;
		}
		DWORD CreatNodes_Default	(WCHAR* URL ,FileDirectoryNode* PrtDirectoryNode)
		{
			WCHAR* FindURL = AddedWildcards(URL);
			WIN32_FIND_DATA findData;
			HANDLE hFind = FindFirstFileW(FindURL, &findData);
			if (hFind == INVALID_HANDLE_VALUE)
			{
				DWORD TD = GetLastError();
				if (TD != 5)
				{
					// Ȩ�޲���
					return -1;
				}
			}
			do {
				// ����û�����б� û�����б��ʾΪ�ļ� ֱ�ӷ��ش�С
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT || findData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
				{
					// �ص����Ŀ¼ ����ʵ���ļ�
				}
				else if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// ����ͨ�ļ�
					FileDirectoryNode* NewNode = new FileDirectoryNode; 
					NewNode->cFileName = new WCHAR[wcslen(findData.cFileName) + 1];
					NewNode->Attribute = findData.dwFileAttributes;
					NewNode->PrtDirectory = PrtDirectoryNode;
					wmemcpy(NewNode->cFileName, findData.cFileName, wcslen(findData.cFileName) + 1);

					// �����ļ���С
					NewNode->FileSize = (static_cast<UINT64>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;

					// �򸸽ڵ㴫��
					PrtDirectoryNode->FileSize += NewNode->FileSize;
					PrtDirectoryNode->AddSublistItem(NewNode);
				}
				else
				{
					// ���ļ���
					if (wcscmp(findData.cFileName, L".") && wcscmp(findData.cFileName, L".."))
					{
						// �����ӽڵ�
						FileDirectoryNode* NewNode = new FileDirectoryNode;
						NewNode->Attribute = findData.dwFileAttributes;
						NewNode->cFileName = new WCHAR[wcslen(findData.cFileName) + 1];
						NewNode->PrtDirectory = PrtDirectoryNode;
						wmemcpy(NewNode->cFileName, findData.cFileName, wcslen(findData.cFileName) +1);

						// �ݹ�
						WCHAR* SubUrl = ConnectionFileName(URL, findData.cFileName);
						CreatNodes_Default(SubUrl ,NewNode);

						// Ϊ�˸�����������
						PrtDirectoryNode->FileSize += NewNode->FileSize;
						PrtDirectoryNode->AddSublistItem(NewNode);
						delete[] SubUrl;
					}
				}
			} while (FindNextFileW(hFind, &findData) != 0);

			// �ڴ��ͷ�
			delete[] FindURL;
			FindClose(hFind);
		
			return 1;
		}

		// �ļ�URlƴ�Ӳ���
		WCHAR* AddedWildcards(WCHAR* StartingPosition)
		{
			WCHAR* NewUrl = new WCHAR[wcslen(StartingPosition) + 3];

			// �ҵ�ʵ��ĩβ
			WCHAR* TempPoint = StartingPosition + wcslen(StartingPosition);
			while (TempPoint >= StartingPosition && ((*(TempPoint - 1) == L'\\') || (*(TempPoint - 1) == L'*'))) --TempPoint;

			// ����
			UINT StartUelLen = TempPoint - StartingPosition;
			wmemcpy(NewUrl, StartingPosition, StartUelLen);
			NewUrl[StartUelLen]		= L'\\';
			NewUrl[StartUelLen+1]	= L'*';
			NewUrl[StartUelLen+2]	= L'\0';

			return NewUrl;
		}
		WCHAR* ConnectionFileName(WCHAR* StartingPosition, WCHAR* SubFile)
		{
			// ������
			UINT32 NewLen = wcslen(StartingPosition) + wcslen(SubFile) + 2;
			WCHAR* NewUrl = new WCHAR[NewLen];

			// �ҵ�ʵ��ĩβ
			WCHAR* TempPoint = StartingPosition + wcslen(StartingPosition);
			while (TempPoint >= StartingPosition && ((*(TempPoint - 1) == L'\\') || (*(TempPoint - 1) == L'*'))) --TempPoint;

			// ����
			UINT StartUelLen = TempPoint - StartingPosition;
			wmemcpy(NewUrl, StartingPosition, StartUelLen);
			NewUrl[StartUelLen] = L'\\';
			wmemcpy(&NewUrl[StartUelLen + 1], SubFile, wcslen(SubFile) + 1);

			return NewUrl;
		}
};
#pragma once
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <iostream>


/* 
文件目录类 
重点是磁盘目录指针 
	节点有名称大小子列表
	最上面是ROOT节点没有任何东西 存在DiskFileDirectory 子列表是物理磁盘 没用父节点
主要方法
	建立列表 
		递归 (物理磁盘 逻辑分盘 文件和文件夹) 三个函数以及一个函数作为入口

*/
class FileDirectory
{
	public:
		// 私有的储存整目录的结构体 表示文件的时候节点子链表为null
		class FileDirectoryNode
		{
			friend class FileDirectory;
			public:
				DWORD	Attribute;		// 文件信息 标签
				WCHAR*	cFileName;		// 节点名称 文件名称
				UINT64	FileSize;		// 实际使用的空间
				UINT32 SubfileNum;		// 已确定的子文件数量

				// 文件子目录列表和父级指针
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
				// 增减子列表项
				UINT32 ArrayLen;	// 当前数组的长度
				void AddSublistItem(FileDirectoryNode* newItem)
				{
					// 判断是否超出容量
					if (SubfileNum >= ArrayLen)
					{
						// 新的数组长度 分配新数组
						UINT newLen = ArrayLen + 10;
						FileDirectoryNode** newArray = new FileDirectoryNode * [newLen];

						// 把旧的数据复制到新数组
						if (SubDirectory != nullptr)
						{
							memcpy(newArray, SubDirectory, SubfileNum * sizeof(FileDirectoryNode*));
							delete[] SubDirectory;
						}

						// 更新指针
						SubDirectory = newArray;
						ArrayLen = newLen;
					}

					// 添加新元素
					SubDirectory[SubfileNum] = newItem;
					SubfileNum++;
				}
		};
		class FileDirectoryNode_A : public FileDirectoryNode 
		{
			public:
				UINT64	QuotaSize;		// 配额大小 比如磁盘大小分区大小

				FileDirectoryNode_A()
				{
					QuotaSize = 0;
				}
		};




	public:
		FileDirectoryNode* DiskFileDirectory;	// 这个磁盘的目录
		UINT32 NodeCounter;						// 计数器






		// 创建新列表 函数入口 创建根节点 
		DWORD InitializeDirectory()
		{
			// 创建根节点
			if (DiskFileDirectory) delete DiskFileDirectory;
			DiskFileDirectory = new FileDirectoryNode;

			// 调用
			CreatNodes_Volume(NULL,DiskFileDirectory);

			return 1;
		}
		




		// 排序子列表 懒得做了直接冒泡
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

		/* ------------- 创建文件目录部分 ------------- */

		// 物理磁盘遍历 为父节点创建子节点（父级节点地址）
		DWORD CreatNodes_Disk		(WCHAR* URL ,FileDirectoryNode* PrtDirectoryNode)
		{
			for(int i = 0 ; i < 32 ; i++)
			{
				// 物理磁盘编号
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

				// 物理磁盘存在则创建子节点
				FileDirectoryNode_A* pDisk = new FileDirectoryNode_A;
				pDisk->PrtDirectory = PrtDirectoryNode;
				PrtDirectoryNode->AddSublistItem(pDisk);

				// 取得磁盘容量
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
				
				// 取得名称
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

				// 释放 
				CloseHandle(hDevice);
			}
			return 0;
		}
		DWORD CreatNodes_Volume		(WCHAR* URL ,FileDirectoryNode* PrtDirectoryNode)
		{
			//遍历所有分卷 父节点还是根节点URl参数保持一致性选择保留
			WCHAR volumeName[MAX_PATH];
			HANDLE hFind = FindFirstVolumeW(volumeName, MAX_PATH);
			if (hFind == INVALID_HANDLE_VALUE) return 0;
			do 
			{
				// 打开卷
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
					// （1 创建子节点
					FileDirectoryNode_A* pVolume = new FileDirectoryNode_A;
					pVolume->PrtDirectory = PrtDirectoryNode;
					PrtDirectoryNode->AddSublistItem(pVolume);
					
					// （2 记录挂载点名称 和大小
					WCHAR volumePaths[MAX_PATH] = { 0 };
					DWORD returnLength = 0;
					volumeName[len - 1] = L'\\';
					if (GetVolumePathNamesForVolumeNameW(volumeName, volumePaths, MAX_PATH, &returnLength))
					{
						// 记录挂载点 只选取第一个
						pVolume->cFileName = new WCHAR[wcslen(volumePaths) + 1];
						wmemcpy(pVolume->cFileName, volumePaths, wcslen(volumePaths) + 1);

						// 获取容量 / 已用 / 剩余
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
								// 递归 遍历文件
								CreatNodes_Default(volumePaths, pVolume);
							}
						}
						// DWORD ERROR_ = GetLastError();
						Sleep(1);
					}
					// 释放
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
					// 权限不够
					return -1;
				}
			}
			do {
				// 看有没有子列表 没有子列表表示为文件 直接返回大小
				if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT || findData.dwFileAttributes & FILE_ATTRIBUTE_DEVICE)
				{
					// 重点解析目录 不是实际文件
				}
				else if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
				{
					// 是普通文件
					FileDirectoryNode* NewNode = new FileDirectoryNode; 
					NewNode->cFileName = new WCHAR[wcslen(findData.cFileName) + 1];
					NewNode->Attribute = findData.dwFileAttributes;
					NewNode->PrtDirectory = PrtDirectoryNode;
					wmemcpy(NewNode->cFileName, findData.cFileName, wcslen(findData.cFileName) + 1);

					// 计算文件大小
					NewNode->FileSize = (static_cast<UINT64>(findData.nFileSizeHigh) << 32) | findData.nFileSizeLow;

					// 向父节点传入
					PrtDirectoryNode->FileSize += NewNode->FileSize;
					PrtDirectoryNode->AddSublistItem(NewNode);
				}
				else
				{
					// 是文件夹
					if (wcscmp(findData.cFileName, L".") && wcscmp(findData.cFileName, L".."))
					{
						// 创建子节点
						FileDirectoryNode* NewNode = new FileDirectoryNode;
						NewNode->Attribute = findData.dwFileAttributes;
						NewNode->cFileName = new WCHAR[wcslen(findData.cFileName) + 1];
						NewNode->PrtDirectory = PrtDirectoryNode;
						wmemcpy(NewNode->cFileName, findData.cFileName, wcslen(findData.cFileName) +1);

						// 递归
						WCHAR* SubUrl = ConnectionFileName(URL, findData.cFileName);
						CreatNodes_Default(SubUrl ,NewNode);

						// 为了父级填入数据
						PrtDirectoryNode->FileSize += NewNode->FileSize;
						PrtDirectoryNode->AddSublistItem(NewNode);
						delete[] SubUrl;
					}
				}
			} while (FindNextFileW(hFind, &findData) != 0);

			// 内存释放
			delete[] FindURL;
			FindClose(hFind);
		
			return 1;
		}

		// 文件URl拼接操作
		WCHAR* AddedWildcards(WCHAR* StartingPosition)
		{
			WCHAR* NewUrl = new WCHAR[wcslen(StartingPosition) + 3];

			// 找到实际末尾
			WCHAR* TempPoint = StartingPosition + wcslen(StartingPosition);
			while (TempPoint >= StartingPosition && ((*(TempPoint - 1) == L'\\') || (*(TempPoint - 1) == L'*'))) --TempPoint;

			// 复制
			UINT StartUelLen = TempPoint - StartingPosition;
			wmemcpy(NewUrl, StartingPosition, StartUelLen);
			NewUrl[StartUelLen]		= L'\\';
			NewUrl[StartUelLen+1]	= L'*';
			NewUrl[StartUelLen+2]	= L'\0';

			return NewUrl;
		}
		WCHAR* ConnectionFileName(WCHAR* StartingPosition, WCHAR* SubFile)
		{
			// 新数组
			UINT32 NewLen = wcslen(StartingPosition) + wcslen(SubFile) + 2;
			WCHAR* NewUrl = new WCHAR[NewLen];

			// 找到实际末尾
			WCHAR* TempPoint = StartingPosition + wcslen(StartingPosition);
			while (TempPoint >= StartingPosition && ((*(TempPoint - 1) == L'\\') || (*(TempPoint - 1) == L'*'))) --TempPoint;

			// 复制
			UINT StartUelLen = TempPoint - StartingPosition;
			wmemcpy(NewUrl, StartingPosition, StartUelLen);
			NewUrl[StartUelLen] = L'\\';
			wmemcpy(&NewUrl[StartUelLen + 1], SubFile, wcslen(SubFile) + 1);

			return NewUrl;
		}
};
#include "UsnManage.h"

namespace quicksearch
{
#define BUFF_SIZE 4096
	// 初始化USN管理
	bool UsnManage::Init()
	{
		if (isNTFS(rootPath_.c_str()))
		{
			return getVolumeHandle(rootPath_.c_str())
				&& createUSN()
				&& getBasicInfo();
		}
		else
			return false;
	}

	bool UsnManage::ChangeRecord(std::vector<util::ChangeInfo>& changeRecord)
	{
		DWORD dwBytes;
		DWORD dwRetBytes;
		READ_USN_JOURNAL_DATA_V0 ReadData;
		ZeroMemory(&ReadData, sizeof(ReadData));
		// 文件创建 更名 删除
		ReadData.ReasonMask =
			USN_REASON_FILE_CREATE |
			USN_REASON_FILE_DELETE |
			USN_REASON_RENAME_NEW_NAME |
			USN_REASON_RENAME_OLD_NAME;
		ReadData.UsnJournalID = USNInfo_.UsnJournalID;

		PUSN_RECORD_V2 UsnRecord;
		CHAR Buffer[BUFF_SIZE];

		if (DeviceIoControl(hd_, 
			FSCTL_READ_USN_JOURNAL, 
			&ReadData, 
			sizeof(ReadData), 
			&Buffer, 
			BUFF_SIZE, 
			&dwBytes, 
			NULL))
		{
			dwRetBytes = dwBytes - sizeof(USN);
			UsnRecord = (PUSN_RECORD_V2)(((PUCHAR)Buffer) + sizeof(USN));

			// FileReferenceNumber 作为key 保存文件信息
			std::unordered_map<DWORDLONG, util::ChangeInfo> record;

			while (dwRetBytes > 0)
			{

				const int strLen = UsnRecord->FileNameLength;
				char fileName[MAX_PATH] = { 0 };
				WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);


				TCHAR path[MAX_PATH] = { 0 };
				TCHAR pPath[MAX_PATH] = { 0 };
				DWORD dwRet;

				// 获取文件句柄

				HANDLE hFile = OpenFileById(hd_,
					&getFileIdDescriptor(UsnRecord->FileReferenceNumber),
					SYNCHRONIZE | FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					FILE_FLAG_BACKUP_SEMANTICS);

				HANDLE phFile = OpenFileById(hd_,
					&getFileIdDescriptor(UsnRecord->ParentFileReferenceNumber),
					SYNCHRONIZE | FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					FILE_FLAG_BACKUP_SEMANTICS);

				// 如果文件打开不了 说明已经被删除
				// 获得当前文件及其父目录状态
				if (hFile != INVALID_HANDLE_VALUE && phFile != INVALID_HANDLE_VALUE)
				{
					// 获取文件完整路径
					GetFinalPathNameByHandle(hFile, path, MAX_PATH, VOLUME_NAME_NONE);
					GetFinalPathNameByHandle(phFile, pPath, MAX_PATH, VOLUME_NAME_NONE);
				}
				else
				{
					strcpy(path, "");
				}

				std::string spath = path;
				std::string sppath = pPath;

				std::string fullpath = spath.size() > 0 ? rootPath_ + path : path;
				std::string fullPpath = sppath.size() > 0 ? rootPath_ + pPath : pPath;

				
				// 文件新建
				if (UsnRecord->Reason & USN_REASON_FILE_CREATE)
				{
					record[UsnRecord->FileReferenceNumber].filename_ = fileName;
					record[UsnRecord->FileReferenceNumber].path_ = fullpath;
					record[UsnRecord->FileReferenceNumber].pPath_ = fullPpath;
					record[UsnRecord->FileReferenceNumber].isExist_ = true;
					record[UsnRecord->FileReferenceNumber].isNew_ = true;
				}
				// 文件更名
				else if (UsnRecord->Reason & USN_REASON_RENAME_NEW_NAME)
				{
					record[UsnRecord->FileReferenceNumber].filename_ = fileName;
					record[UsnRecord->FileReferenceNumber].newName_ = fileName;
					record[UsnRecord->FileReferenceNumber].path_ = fullpath;
					record[UsnRecord->FileReferenceNumber].pPath_ = fullPpath;
					record[UsnRecord->FileReferenceNumber].isNew_ = false;
				}
				else if (UsnRecord->Reason & USN_REASON_RENAME_OLD_NAME)
				{
					record[UsnRecord->FileReferenceNumber].oldName_ = fileName;
					record[UsnRecord->FileReferenceNumber].path_ = fullpath;
					record[UsnRecord->FileReferenceNumber].pPath_ = fullPpath;
					record[UsnRecord->FileReferenceNumber].isNew_ = false;
				}
				// 是否被删除
				else if (UsnRecord->Reason & USN_REASON_FILE_DELETE)
				{
					// 删除后的名字 与删除前不一样，所以也会有oldName
					record[UsnRecord->FileReferenceNumber].filename_ = fileName;
					record[UsnRecord->FileReferenceNumber].isExist_ = false;
					record[UsnRecord->FileReferenceNumber].isNew_ = false;
				}

				dwRetBytes -= UsnRecord->RecordLength;
				UsnRecord = (PUSN_RECORD_V2)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}
			// 将文件信息保存到vector中
			for (auto& e : record)
			{
				changeRecord.push_back(e.second);
			}
			ReadData.StartUsn = *(USN*)& Buffer;
		}
		else
		{
			return false;
		}
		return false;
	}

	bool UsnManage::isNTFS(const char* volName)
	{
		char sysNameBuf[MAX_PATH] = { 0 };
		int ret = GetVolumeInformationA(volName,
			NULL,
			0,
			NULL,
			NULL,
			NULL,
			sysNameBuf,
			MAX_PATH);

		if (ret) {
			printf("文件系统名: %s\n", sysNameBuf);

			// 比较字符串
			if (0 == strcmp(sysNameBuf, "NTFS")) {
				return true;
			}
			else {
				printf("该驱动盘非NTFS格式\n");
				return false;
			}

		}
		return false;
	}

	bool UsnManage::getVolumeHandle(const char* volName)
	{
		char fileName[MAX_PATH];
		fileName[0] = '\0';

		// 传入的文件名必须为\\.\C:的形式
		strcpy_s(fileName, "\\\\.\\");
		strcat_s(fileName, volName);
		// 为了方便操作，这里转为string进行去尾
		std::string fileNameStr = (std::string)fileName;
		fileNameStr.erase(fileNameStr.find_last_of(":") + 1);

		printf("驱动盘地址: %s\n", fileNameStr.data());

		// 调用该函数需要管理员权限
		hd_ = CreateFileA(fileNameStr.data(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, // 必须包含有FILE_SHARE_WRITE
			NULL,
			OPEN_EXISTING, // 必须包含OPEN_EXISTING, CREATE_ALWAYS可能会导致错误
			FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL可能会导致错误
			NULL);

		if (hd_ != INVALID_HANDLE_VALUE) {
			return true;
		}
		else {
			std::cout << "获取驱动盘句柄失败 —— handle :" << hd_ << GetLastError() << std::endl;
			return false;
		}
		return false;
	}

	bool UsnManage::createUSN()
	{
		DWORD br;
		CREATE_USN_JOURNAL_DATA cujd;
		cujd.MaximumSize = 0; // 0表示使用默认值
		cujd.AllocationDelta = 0; // 0表示使用默认值
		int ret = DeviceIoControl(hd_,
			FSCTL_CREATE_USN_JOURNAL,
			&cujd,
			sizeof(cujd),
			NULL,
			0,
			&br,
			NULL);

		if (ret) {
			return true;
		}
		else {
			printf("初始化USN日志文件失败 —— status:%x error:%d\n", ret, GetLastError());
		}
		return false;
	}


	bool UsnManage::getBasicInfo()
	{
		DWORD br;
		int ret = DeviceIoControl(hd_,
			FSCTL_QUERY_USN_JOURNAL,
			NULL,
			0,
			&USNInfo_,
			sizeof(USN_JOURNAL_DATA),
			&br,
			NULL);

		if (ret) {
			return true;
		}
		else {
			printf("获取USN日志基本信息失败 —— status:%x error:%d\n", ret, GetLastError());
		}
		return false;
	}

	bool UsnManage::GetRecord(std::vector<FRInfo>& fileInfos_)
	{
		long counter = 0;
		// 根据windos版本
		MFT_ENUM_DATA med;
		med.StartFileReferenceNumber = 0;
		med.LowUsn = 0;
		med.HighUsn = USNInfo_.NextUsn;
		med.MinMajorVersion = 2;

		// 用于储存记录的缓冲
		CHAR buffer[BUFF_SIZE];
		DWORD usnDataSize;
		PUSN_RECORD_V3 UsnRecord;

		while (0 != DeviceIoControl(hd_,
			FSCTL_ENUM_USN_DATA,
			&med,
			sizeof(med),
			buffer,
			BUFF_SIZE,
			&usnDataSize,
			NULL))
		{

			DWORD dwRetBytes = usnDataSize - sizeof(USN);

			// 第一个USN记录 
			UsnRecord = (PUSN_RECORD_V3)(((PCHAR)buffer) + sizeof(USN));

			while (dwRetBytes > 0) {

				const int strLen = UsnRecord->FileNameLength;
				char fileName[MAX_PATH] = { 0 };
				WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);

				TCHAR path[MAX_PATH];
				DWORD dwRet;
				// 获取文件句柄
				HANDLE hFile = INVALID_HANDLE_VALUE;
				hFile = OpenFileById(hd_,
					&getFileIdDescriptor(UsnRecord->FileReferenceNumber),
					SYNCHRONIZE | FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					0);

				// 获取文件完整路径
				dwRet = GetFinalPathNameByHandle(hFile, path, MAX_PATH, VOLUME_NAME_GUID);
				// 路径长度符合规范
				if (dwRet < MAX_PATH)
				{
					FRInfo info;

					info.filename_ = fileName;
					info.path_ = rootPath_ + path;
					info.py_ = Util::ChineseConvertPy(fileName);
					info.py_Abbreviation_ = Util::ChineseConvertPyAbbre(fileName);

					fileInfos_.push_back(info);

					counter++;
				}

				// 获取下一个记录
				DWORD recordLen = UsnRecord->RecordLength;
				dwRetBytes -= recordLen;
				UsnRecord = (PUSN_RECORD_V3)(((PCHAR)UsnRecord) + recordLen);
			}
			med.StartFileReferenceNumber = *(USN*)& buffer;
		}

		return true;
	}

	bool UsnManage::DelUSN()
	{
		DWORD br;
		DELETE_USN_JOURNAL_DATA dujd;
		dujd.UsnJournalID = USNInfo_.UsnJournalID;
		dujd.DeleteFlags = USN_DELETE_FLAG_DELETE;

		int ret = DeviceIoControl(hd_,
			FSCTL_DELETE_USN_JOURNAL,
			&dujd,
			sizeof(dujd),
			NULL,
			0,
			&br,
			NULL);

		if (ret) {
			printf("成功删除USN日志文件!\n");
		}
		else {
			printf("删除USN日志文件失败 —— status:%x error:%d\n", ret, GetLastError());
			return false;
		}
		// 释放句柄
		CloseHandle(hd_);
		hd_ = INVALID_HANDLE_VALUE;
		return true;
	}

	// FILE_ID_128 转 FILE_ID_DESCRIPTOR
	FILE_ID_DESCRIPTOR UsnManage::getFileIdDescriptor(const FILE_ID_128& fileId)
	{

		FILE_ID_DESCRIPTOR fileDescriptor;
		ZeroMemory(&fileDescriptor, sizeof(fileDescriptor));
		fileDescriptor.Type = ExtendedFileIdType;
		fileDescriptor.ExtendedFileId = fileId;
		fileDescriptor.dwSize = sizeof(fileDescriptor);

		return fileDescriptor;
	}
	// DWORDLONG 转 FILE_ID_DESCRIPTOR
	FILE_ID_DESCRIPTOR UsnManage::getFileIdDescriptor(const DWORDLONG fileId)
	{
		FILE_ID_DESCRIPTOR fileDescriptor;
		ZeroMemory(&fileDescriptor, sizeof(fileDescriptor));
		fileDescriptor.Type = FileIdType;
		fileDescriptor.FileId.QuadPart = fileId;
		fileDescriptor.dwSize = sizeof(fileDescriptor);

		return fileDescriptor;
	}


}

#include "UsnManage.h"

namespace quicksearch
{
#define BUFF_SIZE 4096
	// ��ʼ��USN����
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
		// �ļ����� ���� ɾ��
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

			// FileReferenceNumber ��Ϊkey �����ļ���Ϣ
			std::unordered_map<DWORDLONG, util::ChangeInfo> record;

			while (dwRetBytes > 0)
			{

				const int strLen = UsnRecord->FileNameLength;
				char fileName[MAX_PATH] = { 0 };
				WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);


				TCHAR path[MAX_PATH] = { 0 };
				TCHAR pPath[MAX_PATH] = { 0 };
				DWORD dwRet;

				// ��ȡ�ļ����

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

				// ����ļ��򿪲��� ˵���Ѿ���ɾ��
				// ��õ�ǰ�ļ����丸Ŀ¼״̬
				if (hFile != INVALID_HANDLE_VALUE && phFile != INVALID_HANDLE_VALUE)
				{
					// ��ȡ�ļ�����·��
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

				
				// �ļ��½�
				if (UsnRecord->Reason & USN_REASON_FILE_CREATE)
				{
					record[UsnRecord->FileReferenceNumber].filename_ = fileName;
					record[UsnRecord->FileReferenceNumber].path_ = fullpath;
					record[UsnRecord->FileReferenceNumber].pPath_ = fullPpath;
					record[UsnRecord->FileReferenceNumber].isExist_ = true;
					record[UsnRecord->FileReferenceNumber].isNew_ = true;
				}
				// �ļ�����
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
				// �Ƿ�ɾ��
				else if (UsnRecord->Reason & USN_REASON_FILE_DELETE)
				{
					// ɾ��������� ��ɾ��ǰ��һ��������Ҳ����oldName
					record[UsnRecord->FileReferenceNumber].filename_ = fileName;
					record[UsnRecord->FileReferenceNumber].isExist_ = false;
					record[UsnRecord->FileReferenceNumber].isNew_ = false;
				}

				dwRetBytes -= UsnRecord->RecordLength;
				UsnRecord = (PUSN_RECORD_V2)(((PCHAR)UsnRecord) + UsnRecord->RecordLength);
			}
			// ���ļ���Ϣ���浽vector��
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
			printf("�ļ�ϵͳ��: %s\n", sysNameBuf);

			// �Ƚ��ַ���
			if (0 == strcmp(sysNameBuf, "NTFS")) {
				return true;
			}
			else {
				printf("�������̷�NTFS��ʽ\n");
				return false;
			}

		}
		return false;
	}

	bool UsnManage::getVolumeHandle(const char* volName)
	{
		char fileName[MAX_PATH];
		fileName[0] = '\0';

		// ������ļ�������Ϊ\\.\C:����ʽ
		strcpy_s(fileName, "\\\\.\\");
		strcat_s(fileName, volName);
		// Ϊ�˷������������תΪstring����ȥβ
		std::string fileNameStr = (std::string)fileName;
		fileNameStr.erase(fileNameStr.find_last_of(":") + 1);

		printf("�����̵�ַ: %s\n", fileNameStr.data());

		// ���øú�����Ҫ����ԱȨ��
		hd_ = CreateFileA(fileNameStr.data(),
			GENERIC_READ | GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, // ���������FILE_SHARE_WRITE
			NULL,
			OPEN_EXISTING, // �������OPEN_EXISTING, CREATE_ALWAYS���ܻᵼ�´���
			FILE_ATTRIBUTE_READONLY, // FILE_ATTRIBUTE_NORMAL���ܻᵼ�´���
			NULL);

		if (hd_ != INVALID_HANDLE_VALUE) {
			return true;
		}
		else {
			std::cout << "��ȡ�����̾��ʧ�� ���� handle :" << hd_ << GetLastError() << std::endl;
			return false;
		}
		return false;
	}

	bool UsnManage::createUSN()
	{
		DWORD br;
		CREATE_USN_JOURNAL_DATA cujd;
		cujd.MaximumSize = 0; // 0��ʾʹ��Ĭ��ֵ
		cujd.AllocationDelta = 0; // 0��ʾʹ��Ĭ��ֵ
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
			printf("��ʼ��USN��־�ļ�ʧ�� ���� status:%x error:%d\n", ret, GetLastError());
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
			printf("��ȡUSN��־������Ϣʧ�� ���� status:%x error:%d\n", ret, GetLastError());
		}
		return false;
	}

	bool UsnManage::GetRecord(std::vector<FRInfo>& fileInfos_)
	{
		long counter = 0;
		// ����windos�汾
		MFT_ENUM_DATA med;
		med.StartFileReferenceNumber = 0;
		med.LowUsn = 0;
		med.HighUsn = USNInfo_.NextUsn;
		med.MinMajorVersion = 2;

		// ���ڴ����¼�Ļ���
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

			// ��һ��USN��¼ 
			UsnRecord = (PUSN_RECORD_V3)(((PCHAR)buffer) + sizeof(USN));

			while (dwRetBytes > 0) {

				const int strLen = UsnRecord->FileNameLength;
				char fileName[MAX_PATH] = { 0 };
				WideCharToMultiByte(CP_OEMCP, NULL, UsnRecord->FileName, strLen / 2, fileName, strLen, NULL, FALSE);

				TCHAR path[MAX_PATH];
				DWORD dwRet;
				// ��ȡ�ļ����
				HANDLE hFile = INVALID_HANDLE_VALUE;
				hFile = OpenFileById(hd_,
					&getFileIdDescriptor(UsnRecord->FileReferenceNumber),
					SYNCHRONIZE | FILE_READ_ATTRIBUTES,
					FILE_SHARE_READ | FILE_SHARE_WRITE,
					NULL,
					0);

				// ��ȡ�ļ�����·��
				dwRet = GetFinalPathNameByHandle(hFile, path, MAX_PATH, VOLUME_NAME_GUID);
				// ·�����ȷ��Ϲ淶
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

				// ��ȡ��һ����¼
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
			printf("�ɹ�ɾ��USN��־�ļ�!\n");
		}
		else {
			printf("ɾ��USN��־�ļ�ʧ�� ���� status:%x error:%d\n", ret, GetLastError());
			return false;
		}
		// �ͷž��
		CloseHandle(hd_);
		hd_ = INVALID_HANDLE_VALUE;
		return true;
	}

	// FILE_ID_128 ת FILE_ID_DESCRIPTOR
	FILE_ID_DESCRIPTOR UsnManage::getFileIdDescriptor(const FILE_ID_128& fileId)
	{

		FILE_ID_DESCRIPTOR fileDescriptor;
		ZeroMemory(&fileDescriptor, sizeof(fileDescriptor));
		fileDescriptor.Type = ExtendedFileIdType;
		fileDescriptor.ExtendedFileId = fileId;
		fileDescriptor.dwSize = sizeof(fileDescriptor);

		return fileDescriptor;
	}
	// DWORDLONG ת FILE_ID_DESCRIPTOR
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

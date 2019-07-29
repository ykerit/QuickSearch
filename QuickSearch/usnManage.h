#pragma once

#include <Windows.h>
#include <iostream>
#include <string>
#include <vector>
#include "util.h"
#include <unordered_map>

using namespace util;

namespace quicksearch
{
	class UsnManage
	{
	public:
		UsnManage(const std::string& root) : rootPath_(root), hd_(INVALID_HANDLE_VALUE) {}
		bool Init();
		bool ChangeRecord(std::vector<util::ChangeInfo>& changeRecord);
		bool GetRecord(std::vector<FRInfo>& fileInfos_);
		bool DelUSN();
		~UsnManage()
		{
			if (hd_ != INVALID_HANDLE_VALUE)
			{
				CloseHandle(hd_);
			}
		}
	private:
		bool isNTFS(const char* volName);
		bool getVolumeHandle(const char* volName);
		bool createUSN();
		bool getBasicInfo();
		
		FILE_ID_DESCRIPTOR getFileIdDescriptor(const FILE_ID_128& fileId);
		FILE_ID_DESCRIPTOR getFileIdDescriptor(const DWORDLONG fileId);
		
	private:
		HANDLE hd_;
		USN_JOURNAL_DATA USNInfo_ = {0};
		std::string rootPath_;
		
	};
}

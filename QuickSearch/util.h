#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <iostream>

namespace util
{
	// �ļ���Ϣ
	struct FRInfo
	{
		std::string filename_;
		std::string path_;
		std::string py_; // ȫƴ
		std::string py_Abbreviation_; // ����ĸ
	};

	// �޸���Ϣ
	struct ChangeInfo
	{
		std::string filename_;
		std::string oldName_;
		std::string newName_;
		std::string path_;
		std::string pPath_;
		bool isExist_ = false;
		bool isNew_ = false;
	};

    class Util
    {
    public:
        static void GenerateList(const char* path);
		static std::string ChineseConvertPy(const std::string str);
		static std::string ChineseConvertPyAbbre(const std::string& name);
		static void HighLight(const char* str);
    };

	struct LogInfo
	{
		int64_t timestamp_;
		std::string reason_;
		int line_;
		std::string filename_;

		std::string Format()
		{
			return	"Time: " + std::to_string(timestamp_) + 
				"FileName: " + filename_ + 
				"Line: " + std::to_string(line_) + 
				"Reason: " + reason_;
		}
	};

	class Logger
	{
	public:
		Logger()
		{

		}
	private:
		std::vector<LogInfo> logs_;
	};
}

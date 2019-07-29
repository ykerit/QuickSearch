#pragma once
#include "usnManage.h"
#include "database.h"
#include "util.h"
#include <thread>
#include <chrono>

namespace quicksearch
{
	class SearchManage
	{
	public:
		SearchManage() : db_(nullptr), usn_(nullptr), isShutDown_(false) {}
		void InitSearch();
		void Search();
		~SearchManage()
		{
			stopBGDataService();
			if (db_!=nullptr)
			{
				delete db_;
			}
			if (usn_ != nullptr)
			{
				delete usn_;
			}
		}
	private:
		void stopBGDataService();
		void BGDataService();
		void displayResult(const std::vector<FRInfo>& result, std::string& input, std::string& pinyin, std::string& pinyinAbbr);
		void updateDB(std::vector<util::ChangeInfo>& record);
	private:
		DataBase* db_;
		UsnManage* usn_;
		std::vector<FRInfo> records_;
		bool isShutDown_;
	};
}
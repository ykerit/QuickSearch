#pragma once

#include "sqlite3.h"
#include <iostream>
#include <string>
#include <vector>
#include "util.h"
#include <sstream>
#include <unordered_map>
#include <mutex>

using namespace util;
namespace quicksearch
{
	class DataBase
	{
	public:
		DataBase(std::string path) : path_(path) {}
		bool Init();
		bool QueryRecord(std::string pinyin, std::string pinyinAbbreviation, std::vector<FRInfo>& resultSet);
		bool BatchInsert(std::vector<FRInfo>& records);
		bool UpdateRecord(const util::ChangeInfo& info);
		bool DelRecord(const std::string& filename);
		bool insert(const FRInfo& frInfo);
		bool CloseDB()
		{
			sqlite3_close(db_);
			db_ = nullptr;
			return true;
		}

		bool isEmpty();

		~DataBase()
		{
			if (db_ != nullptr)
			{
				CloseDB();
				errMsg_ = nullptr;
			}
		}
		DataBase(const DataBase&) = delete;
		DataBase& operator=(const DataBase&) = delete;
	private:
		bool openDataBase();
		bool createTable();
		sqlite3* db_ = nullptr;
		char* errMsg_ = nullptr;
		std::string path_;
		std::mutex lock_;
	};
}

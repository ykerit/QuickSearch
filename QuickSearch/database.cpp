#include "database.h"

namespace quicksearch
{
	// ������ص�
	static int callback(void* resultSet, int argc, char** argv, char** azColName) {
		FRInfo info;
		std::unordered_map<std::string, std::string> map;
		for (int i = 0; i < argc; i++) {
			map[azColName[i]] = (argv[i] ? argv[i] : "NULL");
		}
		std::unordered_map<std::string, std::string>::iterator iter;
		iter = map.find("name");
		if (iter != map.end())
		{
			info.filename_ = iter->second;
		}
		iter = map.find("path");
		if (iter != map.end())
		{
			info.path_ = iter->second;
		}
		std::vector<FRInfo>* resultSet_ = (std::vector<FRInfo>*)resultSet;
		resultSet_->push_back(info);
		return 0;
	}
	// ���������ص�
	static int count(void* back, int argc, char** argv, char** azColName)
	{
		bool* flag = (bool*)back;
		for (int i = 0; i < argc; i++)
			atoi(argv[i]) > 0 ? *flag = false : *flag = true;
		return 0;
	}

	bool DataBase::Init()
	{
		if (openDataBase())
		{
			if (createTable())
			{
				return true;
			}
		}
		return false;
	}

	//	�����ݿ⣬ ������ݿⲻ���ڣ��򴴽�
	bool DataBase::openDataBase() {

		int result = sqlite3_open_v2(path_.c_str(), &db_,
			SQLITE_OPEN_READWRITE |
			SQLITE_OPEN_CREATE |
			SQLITE_OPEN_NOMUTEX |
			SQLITE_OPEN_SHAREDCACHE,
			nullptr);

		if (result == SQLITE_OK)
		{
			std::clog << "�����ݿ����ӳɹ�" << std::endl;
			return true;
		}
		else {
			std::clog << "�����ݿ�����ʧ��" << std::endl;
		}
		return false;
	}

	// ������
	bool DataBase::createTable() {
		const char* cmd = nullptr;
		cmd = "CREATE TABLE IF NOT EXISTS file("  \
			"id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL," \
			"name           TEXT    NOT NULL," \
			"pinyin			TEXT			," \
			"pinyin_Abbreviation_		TEXT," \
			"path           TEXT    NOT NULL);";

		int res = sqlite3_exec(db_, cmd, NULL, 0, &errMsg_);
		if (res == SQLITE_OK) 
		{
			fprintf(stdout, "Table created successfully\n");
			return true;
		}
		else 
		{
			fprintf(stderr, "SQL error: %s\n", errMsg_);
			sqlite3_free(errMsg_);
			return false;
		}
		return false;
	}

	// ��������
	bool DataBase::insert(const FRInfo& frInfo) {

		// sqlģ��
		const std::string header = "INSERT INTO file (name, pinyin, pinyin_Abbreviation_, path) VALUES (";
		const char comma = ',';
		const char quotation = '\'';

		// ��ȡ�ļ��� ƴ�� ƴ������ĸ ·��
		std::stringstream cmd;
		cmd << header;
		cmd << quotation << frInfo.filename_ << quotation << comma <<
			quotation << frInfo.py_ << quotation << comma <<
			quotation << frInfo.py_Abbreviation_ << quotation << comma <<
			quotation << frInfo.path_ << quotation << ");";

		std::unique_lock<std::mutex> lock(lock_);
		int res = sqlite3_exec(db_, cmd.str().c_str(), NULL, 0, &errMsg_);
		if (res == SQLITE_OK) {
			// fprintf(stdout, "records created successfully\n");
			return true;
		}
		else {
			// fprintf(stderr, "SQL error: %s\n", errMsg_);
			sqlite3_free(errMsg_);
			return false;
		}
		
		return false;
	}

	// ��ѯ����
	bool DataBase::QueryRecord(std::string pinyin, std::string pinyinAbbreviation, std::vector<FRInfo>& resultSet)
	{
		std::string cmd = "SELECT * FROM file WHERE pinyin LIKE '%" + 
			pinyin + "%' OR pinyin_Abbreviation_ LIKE '%" + pinyinAbbreviation + "%';";
		std::unique_lock<std::mutex> lock(lock_);
		int res = sqlite3_exec(db_, cmd.c_str(), callback, &resultSet, &errMsg_);
		if (res == SQLITE_OK) {
			return true;
		}
		else {
			// fprintf(stderr, "SQL error: %s\n", errMsg_);
			sqlite3_free(errMsg_);
			return false;
		}
		return false;
	}

	bool DataBase::BatchInsert(std::vector<FRInfo>& records)
	{
		int res = 0;
		// ��������
		std::unique_lock<std::mutex> lock(lock_);
		res = sqlite3_exec(db_, "begin;", NULL, 0, &errMsg_);

		if (res == SQLITE_OK)
		{
			// �������� ������������
			lock.unlock();
			for (auto& record : records)
			{
				insert(record);
			}
			// �ټ���
			lock.lock();
			//�ύ����
			sqlite3_exec(db_, "commit;", NULL, 0, 0);
			// std::cout << "����������" << std::endl;
			return true;
		}
		else
		{
			// std::cout << "������ʧ��: " << errMsg_ << std::endl;
			sqlite3_free(errMsg_);
			return false;
		}
		return false;
	}


	bool DataBase::UpdateRecord(const util::ChangeInfo& info)
	{
		std::string cmd = "UPDATE file SET name = '"+info.newName_ + "', \
			pinyin = '"+Util::ChineseConvertPy(info.newName_) + "', \
			pinyin_Abbreviation_ = '" + Util::ChineseConvertPyAbbre(info.newName_) + "', \
			path = '" + info.path_ + "' WHERE name = '" +info.oldName_ + "' AND path LIKE '" + info.pPath_ + "%';";
		std::unique_lock<std::mutex> lock(lock_);
		int	res = sqlite3_exec(db_, cmd.c_str(), NULL, 0, &errMsg_);
		if (res == SQLITE_OK) {
			// fprintf(stdout, "operation done successfully\n");
			return true;
		}
		else {
			// fprintf(stderr, "SQL error: %s\n", errMsg_);
			sqlite3_free(errMsg_);
			return false;
		}

		return false;
	}

	// ɾ������
	bool DataBase::DelRecord(const std::string& filename)
	{
		// �����Ŀ¼ɾ��Ŀ¼·�������������
		// �����ļ�ɾ����û��·��ֻ�ܴ����ݿ��н�·��ȡ��
		std::string cmd = "DELETE FROM file WHERE name = '"+ filename + 
			"' OR path LIKE(SELECT path FROM file WHERE name = '"+ filename +"') + '%'";

		std::unique_lock<std::mutex> lock(lock_);
		int	res = sqlite3_exec(db_, cmd.c_str(), NULL, 0, &errMsg_);
		if (res == SQLITE_OK) {
			// fprintf(stdout, "operation done successfully\n");
			return true;
		}
		else {
			// fprintf(stderr, "SQL error: %s\n", errMsg_);
			sqlite3_free(errMsg_);
			return false;
		}
		return false;
	}

	bool DataBase::isEmpty()
	{
		const char* cmd = "SELECT COUNT(id) FROM file;";
		bool flag = false;
		int res = sqlite3_exec(db_, cmd, count, &flag, &errMsg_);
		if (res != SQLITE_OK) {
			// fprintf(stderr, "SQL error: %s\n", errMsg_);
			sqlite3_free(errMsg_);
		}
		return flag;
	}

}

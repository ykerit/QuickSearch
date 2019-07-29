#pragma once

#include <string>
#include "util.h"
#include "database.h"
#include "usnManage.h"

using namespace util;
using namespace quicksearch;

void testPinyin()
{
	std::string key = "dmx";
	// std::string filename = "sd���Ǵ����ǰ�s";
	std::string filename = "��������ɶ��fake";
	std::string pinyin = Util::ChineseConvertPyAbbre(filename);
	int pos;
	if ((pos = pinyin.find(key)) != std::string::npos)
	{

		int len = filename.size();
		int count = 0;
		// ǰ׺
		int i;
		for (i = 0; i < len;)
		{
			// ���ͷ�������ҵ��ģ� ��û��ǰ׺
			if (count == pos)
			{
				break;
			}
			// �������ĸ
			if (filename[i] >= 0 && filename[i] <= 128)
			{
				++i;
				count++;
				continue;
			}
			// ����Ǻ���
			std::string temp; // ���浥������
			temp.push_back(filename[i]);
			temp.push_back(filename[i + 1]);
			temp = Util::ChineseConvertPyAbbre(temp);
			count += temp.size(); // ƴ������
			i += 2;
			if (count >= pos)
			{
				break;
			}
		}
		int j;
		count = 0;
		// ��������
		for (j = i; j < len;)
		{
			if (filename[j] >= 0 && filename[j] <= 128)
			{
				++j;
				count++;
				continue;
			}
			// ����Ǻ���
			std::string temp; // ���浥������
			temp.push_back(filename[j]);
			temp.push_back(filename[j + 1]);
			temp = Util::ChineseConvertPyAbbre(temp);
			count += temp.size(); // ƴ������
			j += 2;
			if (count == key.size())
			{
				break;
			}
		}
		std::cout << filename.substr(0, i);
		Util::HighLight(filename.substr(i, j - i).c_str());
		std::cout << filename.substr(j) << std::endl;
	}

}

void testHanzi()
{
	std::string key = "����";
	std::string filename = "sd������ɶ��fake";
	int pos;
	if ((pos = filename.find(key)) != std::string::npos)
	{
		std::cout << filename.substr(0, pos);
		Util::HighLight(filename.substr(pos, key.size()).c_str());
		std::cout << filename.substr(pos + key.size()) << std::endl;
	}
}

void testChangeRecord()
{
	DataBase db("record.db");
	db.Init();
	UsnManage usn("E:");
	usn.Init();

	std::vector<util::ChangeInfo> record;
	usn.ChangeRecord(record);

	for (auto& e : record)
	{
		// ����ļ��ڻ���վҲ��ɾ��ɾ���� ɾ����¼
		if (!e.isExist_ && e.oldName_.size() > 0)
		{
			db.DelRecord(e.oldName_);
		}
		else
		{
			// �´����ļ�
			if (e.isNew_)
			{
				FRInfo info;
				info.filename_ = e.filename_;
				info.path_ = e.path_;
				info.py_ = Util::ChineseConvertPy(e.filename_);
				info.py_Abbreviation_ = Util::ChineseConvertPyAbbre(e.filename_);
				db.insert(info);
			}
			// ����
			else if (e.oldName_ != e.newName_)
			{
				db.UpdateRecord(e);
			}
		}
	}

	usn.DelUSN();

}
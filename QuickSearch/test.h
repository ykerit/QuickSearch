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
	// std::string filename = "sd我是大明星啊s";
	std::string filename = "大明星是啥啊fake";
	std::string pinyin = Util::ChineseConvertPyAbbre(filename);
	int pos;
	if ((pos = pinyin.find(key)) != std::string::npos)
	{

		int len = filename.size();
		int count = 0;
		// 前缀
		int i;
		for (i = 0; i < len;)
		{
			// 如果头部就是找到的， 就没有前缀
			if (count == pos)
			{
				break;
			}
			// 如果是字母
			if (filename[i] >= 0 && filename[i] <= 128)
			{
				++i;
				count++;
				continue;
			}
			// 如果是汉字
			std::string temp; // 保存单个汉字
			temp.push_back(filename[i]);
			temp.push_back(filename[i + 1]);
			temp = Util::ChineseConvertPyAbbre(temp);
			count += temp.size(); // 拼音长度
			i += 2;
			if (count >= pos)
			{
				break;
			}
		}
		int j;
		count = 0;
		// 高亮部分
		for (j = i; j < len;)
		{
			if (filename[j] >= 0 && filename[j] <= 128)
			{
				++j;
				count++;
				continue;
			}
			// 如果是汉字
			std::string temp; // 保存单个汉字
			temp.push_back(filename[j]);
			temp.push_back(filename[j + 1]);
			temp = Util::ChineseConvertPyAbbre(temp);
			count += temp.size(); // 拼音长度
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
	std::string key = "明星";
	std::string filename = "sd明星是啥啊fake";
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
		// 如果文件在回收站也被删除删除了 删除记录
		if (!e.isExist_ && e.oldName_.size() > 0)
		{
			db.DelRecord(e.oldName_);
		}
		else
		{
			// 新创建文件
			if (e.isNew_)
			{
				FRInfo info;
				info.filename_ = e.filename_;
				info.path_ = e.path_;
				info.py_ = Util::ChineseConvertPy(e.filename_);
				info.py_Abbreviation_ = Util::ChineseConvertPyAbbre(e.filename_);
				db.insert(info);
			}
			// 更名
			else if (e.oldName_ != e.newName_)
			{
				db.UpdateRecord(e);
			}
		}
	}

	usn.DelUSN();

}
#include "searchManage.h"

namespace quicksearch
{
	using namespace util;
// 监控间隔
#define INTERVAL 10

	void SearchManage::InitSearch()
	{
		// 初始化数据库
		db_ = new DataBase("record.db");
		db_->Init();
		// 初始化USN日志
		usn_ = new UsnManage("E:");
		usn_->Init();

		std::thread work(&SearchManage::BGDataService, this);
		work.detach();
	}
	void SearchManage::Search()
	{
		while (true)
		{
			std::string input;
			std::cout << "请输入想搜索的文件或目录" << std::endl;
			std::cin >> input;
			std::string pinyin = Util::ChineseConvertPy(input);
			std::string pinyinAbbr = Util::ChineseConvertPyAbbre(input);
			std::vector<FRInfo> result;
			db_->QueryRecord(pinyin, pinyinAbbr, result);

			displayResult(result, input, pinyin, pinyinAbbr);

		}
	}

	void SearchManage::stopBGDataService()
	{
		isShutDown_ = true;
	}

	// 文件系统监控
	void SearchManage::BGDataService()
	{
		// 第一次检索
		if (db_->isEmpty())
		{
			usn_->GetRecord(records_);
			// 之后批量插入
			db_->BatchInsert(records_);
		}
		else
		{
			while (!isShutDown_)
			{
				// 检查文件系统变化
				std::vector<util::ChangeInfo> record;
				usn_->ChangeRecord(record);
				// 获取后对数据库更新
				if (!record.empty())
				{
					updateDB(record);
				}

				std::this_thread::sleep_for(std::chrono::seconds(INTERVAL));

			}
		}
	}

	void SearchManage::displayResult(const std::vector<FRInfo>& result, 
		std::string& input, 
		std::string& pinyin, 
		std::string& pinyinAbbr)
	{
		int pos;
		for (auto& res : result)
		{
			std::string filename = res.filename_;
			int len = filename.size();
			// 如果直接模糊匹配 直接找到字串高亮;
			if ((pos = res.filename_.find(input)) != std::string::npos)
			{
				std::cout << filename.substr(0, pos);
				Util::HighLight(filename.substr(pos, input.size()).c_str());
				std::cout << filename.substr(pos + input.size()) << std::endl;
				std::cout << "    ";
				std::cout << res.path_ << std::endl;
			}
			else
			{
				// 如果是全拼或者拼音首字母
				// 先转拼音和首字母
				std::string pinyinR = Util::ChineseConvertPy(res.filename_);
				std::string pinyinAbbrR = Util::ChineseConvertPyAbbre(res.filename_);

				if ((pos = pinyinR.find(pinyin)) != std::string::npos)
				{					
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
						temp = Util::ChineseConvertPy(temp);
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
						temp = Util::ChineseConvertPy(temp);
						count += temp.size(); // 拼音长度
						j += 2;
						if (count == pinyin.size())
						{
							break;
						}
					}
					// 输出
					std::cout << filename.substr(0, i);
					Util::HighLight(filename.substr(i, j - i).c_str());
					std::cout << filename.substr(j);
					//std::cout << "    ";
					printf("%50s\n", res.path_.c_str());
					// std::cout << res.path_ << std::endl;
					std::cout << std::endl;
				}
				else if ((pos = pinyinAbbrR.find(pinyinAbbr)) != std::string::npos) // 首字母
				{
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
						if (count == pinyin.size())
						{
							break;
						}
					}
					// 输出
					std::cout << filename.substr(0, i);
					Util::HighLight(filename.substr(i, j - i).c_str());
					std::cout << filename.substr(j);
					/*std::cout << "    ";
					std::cout << res.path_ << std::endl;*/
					printf("%50s\n", res.path_.c_str());
					std::cout << std::endl;
				}
			}
			
		}
	}
	void SearchManage::updateDB(std::vector<util::ChangeInfo>& record)
	{
		for (auto& e : record)
		{
			// 如果文件不存在了 删除文件
			if (!e.isExist_ && e.oldName_.size() > 0)
			{
				db_->DelRecord(e.oldName_);
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
					db_->insert(info);
				}
				// 更名
				else if (e.oldName_ != e.newName_)
				{
					db_->UpdateRecord(e);
				}
			}
		}
	}
}


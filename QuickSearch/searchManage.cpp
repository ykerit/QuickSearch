#include "searchManage.h"

namespace quicksearch
{
	using namespace util;
// ��ؼ��
#define INTERVAL 10

	void SearchManage::InitSearch()
	{
		// ��ʼ�����ݿ�
		db_ = new DataBase("record.db");
		db_->Init();
		// ��ʼ��USN��־
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
			std::cout << "���������������ļ���Ŀ¼" << std::endl;
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

	// �ļ�ϵͳ���
	void SearchManage::BGDataService()
	{
		// ��һ�μ���
		if (db_->isEmpty())
		{
			usn_->GetRecord(records_);
			// ֮����������
			db_->BatchInsert(records_);
		}
		else
		{
			while (!isShutDown_)
			{
				// ����ļ�ϵͳ�仯
				std::vector<util::ChangeInfo> record;
				usn_->ChangeRecord(record);
				// ��ȡ������ݿ����
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
			// ���ֱ��ģ��ƥ�� ֱ���ҵ��ִ�����;
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
				// �����ȫƴ����ƴ������ĸ
				// ��תƴ��������ĸ
				std::string pinyinR = Util::ChineseConvertPy(res.filename_);
				std::string pinyinAbbrR = Util::ChineseConvertPyAbbre(res.filename_);

				if ((pos = pinyinR.find(pinyin)) != std::string::npos)
				{					
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
						temp = Util::ChineseConvertPy(temp);
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
						temp = Util::ChineseConvertPy(temp);
						count += temp.size(); // ƴ������
						j += 2;
						if (count == pinyin.size())
						{
							break;
						}
					}
					// ���
					std::cout << filename.substr(0, i);
					Util::HighLight(filename.substr(i, j - i).c_str());
					std::cout << filename.substr(j);
					//std::cout << "    ";
					printf("%50s\n", res.path_.c_str());
					// std::cout << res.path_ << std::endl;
					std::cout << std::endl;
				}
				else if ((pos = pinyinAbbrR.find(pinyinAbbr)) != std::string::npos) // ����ĸ
				{
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
						if (count == pinyin.size())
						{
							break;
						}
					}
					// ���
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
			// ����ļ��������� ɾ���ļ�
			if (!e.isExist_ && e.oldName_.size() > 0)
			{
				db_->DelRecord(e.oldName_);
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
					db_->insert(info);
				}
				// ����
				else if (e.oldName_ != e.newName_)
				{
					db_->UpdateRecord(e);
				}
			}
		}
	}
}


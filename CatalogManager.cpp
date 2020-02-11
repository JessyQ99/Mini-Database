#include"CatalogManager.h"
//构造函数
CatalogManager::CatalogManager() : BM(128)
{
	FILE *fp = fopen(FILE_PATH, "r");
	if(fp == NULL) {
		fp = fopen(FILE_PATH, "w");
		fclose(fp);
		BM.setFileInfoInitialized(FILE_PATH);
	}
}
//析构函数
CatalogManager::~CatalogManager()
{

}

//创建一张新的表
void CatalogManager::createTable(TableMeta meta)
{
	//检验表是否存在
	if (getTablePlace(meta.tableName).first != -1)
		throw table_already_exit_error(meta.tableName);
	int recordLength;
	//获取主键和属性的数目
	int PKNum=meta.primaryKeyVec.size();
	int attrNum=meta.attributeVec.size();
	//计算这条表的记录长度=3个整型变量的字节+表名称所占字节数+主键数目*主键记录所占字节数+属性数目*属性记录所占字节数
	//记录定长存储 其中属性记录字节数=属性最大字符数+2*1byte(bool型变量)+整型type变量=MAX_LENGTH+6
	recordLength = 3 * 4 + MAX_LENGTH + meta.primaryKeyVec.size() * MAX_LENGTH + meta.attributeVec.size() * (MAX_LENGTH+6);
	
	//遍历所有的block寻找合适的能存储记录blockid
	for (int blockId = 0;; blockId++)
	{
		//获取buffer中对应相应块号的页编号
		int frameId = BM.getPageId(FILE_PATH, blockId);
		unsigned  char*buffer = BM.getBuffer(frameId);
		//跳过最开头的记录已使用字节数的整型变量
		int length = BM.getUsedBit(frameId);
		//记录太长无法写入相应的block 
		//前4个字节记录存放记录的大小
		if (length + recordLength > PAGESIZE - 4)
		{
			continue;
		}
		//原文件中新的块的建立 更改块号
		if (blockId > BM.fileAttribute[FILE_PATH].blockNum - 1)
			BM.fileAttribute[FILE_PATH].blockNum = blockId + 1;
		//增加一条新的记录
		BM.fileAttribute[FILE_PATH].recordNum++;
		BM.setUsedBit(frameId,recordLength + length);
		int location = length+4;
		//依次记录表总记录的长度、表的主键数目、表的属性数目
		memcpy(buffer+location, &recordLength, sizeof(int));
		memcpy(buffer+location+4, &PKNum, sizeof(int));
		memcpy(buffer+location + 8, &attrNum, sizeof(int));
		location += 12;
		//记录表的名称
		memcpy(buffer + location, meta.tableName.c_str(), meta.tableName.length()+1);
		location += MAX_LENGTH;
		//记录表的所有主键
		for (int i = 0; i < PKNum; i++)
		{
			memcpy(buffer + location + i * MAX_LENGTH, meta.primaryKeyVec[i].c_str(), meta.primaryKeyVec[i].length()+1);
		}
		location += PKNum * MAX_LENGTH;
		//记录表的所有属性
		for (int i = 0; i < attrNum; i++)
		{
			int j = location + i *(MAX_LENGTH+6);
			//记录属性名称
			memcpy(buffer+j, meta.attributeVec[i].name.c_str(),meta.attributeVec[i].name.length()+1);
			//记录属性unique属性
			buffer[j + MAX_LENGTH] = meta.attributeVec[i].isUnique ? '1' : '0';
			//记录属性是否是index
			buffer[j + MAX_LENGTH+1] = meta.attributeVec[i].isIndex ? '1' : '0';
			//记录属性的类型
			int type = meta.attributeVec[i].type;
			memcpy(buffer + j+MAX_LENGTH + 2, &type, sizeof(int));

		}
		location += attrNum * (MAX_LENGTH+6);
		//设置此页面刚刚被引用过
		BM.setReferenced(frameId);
		//修改过页面页面设置为脏页
		BM.setDirty(frameId);
		
		break;
	}
	
}


//删掉一张表
void CatalogManager::dropTable(string tableName)
{
	int blockId;
	unsigned char*buffer;
	//检验表是否存在
	pair<int,int>location = getTablePlace(tableName);
	int frameId = location.first;
	//此处约定usedbit特指存放数据记录所占字节数 注意实际文件开头四字节是记录数据大小的整型变量
	int usedBit = BM.getUsedBit(frameId);
	//不存在抛出表不存在的异常
	if ( frameId== -1)
		throw table_illegal_error(tableName);
	else
	{		
		BM.fileAttribute[FILE_PATH].recordNum--;
		buffer = BM.getBuffer(frameId);
		int length = BM.getUsedBit(frameId);
		
		//start表示据此块开头的偏移量
		int start = location.second;
		//计算此表的长度
		int recordLength = *(int*)(string(buffer + start, buffer + start + 4).c_str());
		//在缓冲区中更新使用字节数的数据
		BM.setUsedBit(frameId, length - recordLength);
		int i = 0;
		int j;
		//将之后的记录前移
		for(i=start,j=start+recordLength;j<usedBit+4;i++,j++)
		{
			buffer[i] = buffer[j];
		}
		//清除多余记录
		while (i < usedBit + 4)
		{
			buffer[i++] = '\0';
		}
		
	}
	//设置此页面刚刚被引用过
	BM.setReferenced(location.first);
	//修改过的页面设为脏页
	BM.setDirty(location.first);
}

//辅助函数，获取表的位置，返回值是（所在块号，距开头偏移量）
pair<int,int> CatalogManager::getTablePlace(string tableName)
{
	int blockId;
	int start = 4;
	int i, j;
	int offset;
	
	BM.getPageId(FILE_PATH, 0);
	
	for (blockId = 0;blockId < BM.fileAttribute[FILE_PATH].blockNum; blockId++)
	{
		start = 4;
		int frameId = BM.getPageId(FILE_PATH, blockId);
		unsigned char*buffer = BM.getBuffer(frameId);
		int usedBit = BM.getUsedBit(frameId);
		
		while (start!=usedBit+4)//达到文件的末尾
		{
			i = 0;
			//计算表记录的长度
			offset = *(int*)(string(buffer + start, buffer + start + 4).c_str());
			//获取表名
			string t = string(buffer + start + 12, buffer + start + 12+MAX_LENGTH);
			if (trim(t) == tableName)
			{
				//引用此页设置为真
				BM.setReferenced(frameId);
				//找到记录返回表所在的块号和距开头的偏移量
				return make_pair(frameId, start);
			}
			//否则跳转表长度
			
			start += offset;
			
		}
	}
	//没有找到返回-1
	
	return make_pair(-1,-1);
}


//找到（表，记录）的位置 与getTablePlace类似
pair<int, int> CatalogManager::getAttrPlace(string tableName, string attrName)
{
	int blockId;
	int start = 0;
	int i;
	pair<int, int>tblLocation;
	//先检验表是否存在获取表的位置
	tblLocation = getTablePlace(tableName);
	if (tblLocation.first != -1)
	{
		unsigned char*buffer = BM.getBuffer(tblLocation.first);
		int PKNum = *(int*)(string(buffer + tblLocation.second + 4, buffer + tblLocation.second + 8).c_str());
		int attrNum = *(int*)(string(buffer + tblLocation.second + 8, buffer + tblLocation.second + 12).c_str());

		start = tblLocation.second + 12 + MAX_LENGTH + PKNum * MAX_LENGTH;
		for (int i = 0; i < attrNum; i++)
		{
			//检验属性名是否存在
			if (trim(string(buffer + start + i * (MAX_LENGTH + 6), buffer + start + i * (MAX_LENGTH + 6) + MAX_LENGTH)) == attrName)
			{
				return make_pair(tblLocation.first, start + i * (MAX_LENGTH + 6));
			}
		}
	}

	return make_pair(-1, -1);
}


//增加索引 修改索引标记为true
void CatalogManager::addIndex(string table, string attribute)
{
	pair<int, int>location = getAttrPlace(table, attribute);
	//非法属性
	if (location.first == -1)
		throw attribute_illegal_error();

	unsigned char*buffer = BM.getBuffer(location.first);
	//计算属性变量所在的位置
	int offset = location.second + MAX_LENGTH + 1;
	//可能有问题..索引已经存在？？如果索引确实存在但add不同name的索引可以吗？
	if (buffer[offset] == '1')
		throw index_already_exit_error();
	//修改索引标记为真
	else
		buffer[offset] = '1';
	//标志脏页
	BM.setDirty(location.first);
}

//去除索引 修改索引标记为false 同上
void CatalogManager::removeIndex(string table, string attribute)
{
	pair<int, int>location = getAttrPlace(table, attribute);
	if (location.first == -1)
		throw attribute_illegal_error();
	unsigned char*buffer = BM.getBuffer(location.first);
	int offset = location.second + MAX_LENGTH + 1;
	if (buffer[offset] == '0')
		throw index_already_exit_error();
	else
		buffer[offset] = '0';
	BM.setDirty(location.first);
}

//获取表的元数据
TableMeta CatalogManager::getMeta(string tableName)
{
	TableMeta result;
	pair<int, int>location = getTablePlace(tableName);
	
	if (location.first == -1)
		throw table_illegal_error(tableName);
	
	result.tableName = tableName;
	unsigned char*buffer = BM.getBuffer(location.first);
	int start = location.second;
	
	int PKNum = *(int*)(string(buffer+start+ 4, buffer+start + 8).c_str());
	int attrNum = *(int*)(string(buffer+start + 8, buffer+start + 12).c_str());
	start += 12+MAX_LENGTH;
	//添加主键
	for (int i = 0; i < PKNum; i++)
	{
		string s = trim(string(buffer + start + i * MAX_LENGTH, buffer + start + (i + 1)*MAX_LENGTH));
		result.primaryKeyVec.push_back(s);
	}
	start += PKNum* MAX_LENGTH;
	//添加属性
	for (int i = 0; i < attrNum; i++)
	{
		Attribute t;
		int k = start + i* (MAX_LENGTH + 6);
		t.name = trim(string(buffer+k, buffer+k + 20));
		t.isUnique = buffer[k + MAX_LENGTH] - '0';
		t.isIndex = buffer[k + MAX_LENGTH+1] - '0';
		t.type = *(int*)(string(buffer + k+MAX_LENGTH+2, buffer + k+MAX_LENGTH+6).c_str());
		result.attributeVec.push_back(t);
		
	}
	
	return result;
}

//检测是否存在表
bool CatalogManager::isExistTable(string tableName)
{
	return getTablePlace(tableName).first == -1 ? false : true;
}

//检验是否存在属性
bool CatalogManager::isExistIndex(string tableName, string attribute)
{
	pair<int, int>location = getAttrPlace(tableName, attribute);
	if (location.first == -1)
		return false;
	unsigned char*buffer = BM.getBuffer(location.first);
	//找到属性idnex标记的位置
	int offset = location.second + MAX_LENGTH + 1;
	return buffer[offset] - '0';
}
//检测是否存在属性
bool CatalogManager::isExistAttribute(string tableName, string attribute)
{
	return getAttrPlace(tableName, attribute).first == -1 ? false : true;
}
//检测属性是否设定成unique
bool CatalogManager::isAttrUnique(string tableName, string attribute)
{
	pair<int, int>location = getAttrPlace(tableName, attribute);
	if (location.first == -1)
		return false;
	unsigned char*buffer = BM.getBuffer(location.first);
	//找到属性unique标记的位置
	int offset = location.second + MAX_LENGTH;
	return buffer[offset] - '0';
}

//获取属性对应的类型
TYPE CatalogManager::getType(string tableName, string attributeName)
{
	pair<int, int>attrLocation = getAttrPlace(tableName, attributeName);
	if(attrLocation.first == -1)
		throw Attribute_Not_Exist_Error(attributeName);
	unsigned char*buffer = BM.getBuffer(attrLocation.first);
	TYPE type = *(int*)(string(buffer + attrLocation.second + MAX_LENGTH+2, buffer + attrLocation.second + MAX_LENGTH+6).c_str());
	return type;
}

//去除string末尾多余的'\0'
string trim(string t)
{
	int i;
	for (i = 0; i < t.length(); i++)
		if (!t[i])
			break;
	return t.substr(0,i);
}
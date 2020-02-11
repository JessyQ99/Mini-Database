#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <fstream>
#include "DS.h"
#include "BplusTree.h"
#include "BufferManager.h"
#include "IndexString.h"

using namespace std;

// 利用B+树创建、删除索引，并支持查询和删除操作
class IndexManager
{
public:
	//构造函数，在构造的时候直接将B+树的部分内容读入磁盘，提升效率
	IndexManager(BufferManager &TM);

	//析构函数，在析构的时候通过b+树的方法将内存中暂存的节点和配置信息存放到磁盘中
	~IndexManager();
	
	// 为表创建索引
	void createIndex(Index _index, TYPE type);
	
	// 删除索引
	void dropIndex(string indexName);
	
	// 索引搜索, 为了速度，直接传索引了，搜索的结果以指针的形式存放在文件中
	int SearchRetFile(string tableName, Condition condition);
	
	// 单条搜索索引，返回一个符合条件的指针
	Pointer SearchRetKey(string tablename, Condition condition);
	
	// 删除index中的某一个Key
	void deleteKey(string tableName, string Attr, Data key);
	
	// 删除之后更新key对应的数据的指针的值
	void updateKey(string tableName, string Attr, Data key, Pointer _ptr);
	
	// 插入key值到B+树中
	void insertKey(string tableName, string Attr, Data key, Pointer _ptr);
	
	// 返回索引根据索引名返回索引信息
	Index getIndex(string indexName);

	// 删除所有该表的索引
	void dropTableIndex(string tableName);
private:
	int _block_size = 4096;
	string _conf_path = "./index/conf";
	char _ret_file[1024] = "./record/temp";
	// 每一个B+树对应的配置文件
    struct bplus_tree_config {
        char filename[1024];
        int type;
		string tableName;
		string attribute;
		string indexName;
    };

	// 将buffer中的ptr信息转换到存放的64位信息
	long long generateData(Pointer _ptr);

	// 从Attr获取到变量的长度
	int getAttrSize(Attribute Attr);

	// int类型的range搜索
	int int_get_range(bplus_tree *tempTree, Condition condition);

	// float类型的range搜索
	int float_get_range(bplus_tree *tempTree, Condition condition);

	// string类型的range搜索
	int string_get_range(bplus_tree *tempTree, Condition condition);

	// 将64位的存放信息转换到buffer中的Ptr信息
	Pointer fromData2Ptr(long long data);

	// 用于存放树的列表和它们对应的Index
    map<string, bplus_tree_config> treeList;

	// 存放当前活跃的B+树的根节点
	map<string, bplus_tree *> activeTreeList;

	// int和float类型对应的长度是一定的，所以提前激活他们对应的B+树对象
	BplusTree<int> intTree;

	BplusTree<float> floatTree;

	BplusTree<IndexString> stringTree;

	BufferManager &TM;
};

#endif
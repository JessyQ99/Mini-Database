//CatalogManager.h
//存储表的相关信息
#define _CRT_SECURE_NO_WARNINGS
#ifndef CATALOGMANAGER_H
#define CATALOGMANAGER_H

#include "DS.h"
#include"BufferManager.h"
#include"exception.h"

#define FILE_PATH "./catalog/tableInfo"
// #define FILE_PATH "E://kk.txt"
#define MAX_LENGTH 20
// 管理表的元数据文件
// 支持创建和删除
// 提供一些访问元数据的接口
string trim(string t);
class CatalogManager
{
public:
	CatalogManager();
	~CatalogManager();
	// 根据TableMeta类型的数据创建一张表，返回是否创建成功，不成功的话要抛出对应的异常
	void createTable(TableMeta cata);
	// 接收表名，删除表的元信息
	// drop table
	void dropTable(string tableName);
	// 添加索引信息，就是把一个属性的isIndex设为true
	void addIndex(string table, string attribute);
	// 删除索引信息，即把一个属性的isIndex设为false
	void removeIndex(string table, string attribute);
	// 获得表的元数据
	TableMeta getMeta(string tableName);
	// 检测一个表是否存在
	bool isExistTable(string tableName);
	// 检测一个表的一个属性是否存在
	bool isExistAttribute(string tableName, string attributeName);
	// 检测一个表的一个属性是否存在索引
	bool isExistIndex(string table, string attribute);
	//检测属性是否具有唯一标识符
	bool isAttrUnique(string table, string attribute);
	// 返回该属性的数据类型
	TYPE getType(string tableName, string attributeName);

private:
	BufferManager BM;
	//辅助函数，获得表的位置 返回信息<blockid,offset(距块开始位置的偏移字节）>
	pair<int,int> getTablePlace(string tableName);
	//辅助函数，获得对应表属性的位置 返回信息<blockid,offset(距块开始位置的偏移字节)>
	pair<int, int> getAttrPlace(string tableName,string attrName);
};

#endif
#ifndef RECORDMANAGER_H
#define RECORDMANAGER_H

#include "DS.h"
#include "BufferManager.h"
#include "IndexManager.h"
#include <cstdio>

Pointer loadPtr(unsigned char *buffer, int ptrNo);

class RecordManager
{
public:
	// 功能：ReocrdManger的构造函数，在record目录下创建工作区文件
	// 输入：索引和缓冲区对象的引用
	// 输出：无
	// 返回值：无
	RecordManager(IndexManager &IM, BufferManager &TM) : BM(256), TM(TM), IM(IM) {
		const string filePath = "./record/temp";

    	// 检测文件是否存在， 不存在创建文件
    	FILE *fp = fopen(filePath.c_str(), "r");
    	if(fp == NULL) {
        	fp = fopen(filePath.c_str(), "w");
    	}
    	fclose(fp);
	};
	~RecordManager();
	// 功能：支持单索引多条件的理论无限数量级查询
	// 输入：表的元数据、查询条件
	// 输出：符合条件的记录
	// 返回值：符合条件的记录数量
	int select(TableMeta meta, Where where);
	// 功能：无约束时间复杂度为O(1)的插入
	// 输入：表的元数据、要插入的元组
	// 输出：无
	// 返回值：插入成功的条数（一般为1）
	int insert(TableMeta meta, Tuple tuple);
	// 功能：支持多条件的删除
	// 输入：表的元数据、删除条件
	// 输出：无
	// 返回值：被删除的记录的数量
	int deLete(TableMeta meta, Where where);
	// 功能：创建表，在record目录下生成记录文件
	// 输入：表的元数据
	// 输出：无
	// 返回值：是否创建成功，1表示成功，0表示失败
	int createTable(TableMeta meta);
	// 功能：删除表，将记录删除后，把表的文件删除并清理缓冲区
	// 输入：所要删除的表的名字
	// 输出：无
	// 返回值：输出的记录数
	int dropTable(string tableName);
	// 功能：将已有记录插入到B+树中
	// 输入：表的元数据和需要建索引的属性名
	// 输出：无
	// 返回值：插入B+树中Key的数量
	int createIndex(TableMeta meta, string indexAttr);
private:
	int getRecordLength(TableMeta meta);
	// 思路，这个函数用于单条件搜索，搜索结果是块号和序号
	// 写在哪里呢？写在temp文件里
	// 如何求与呢？对temp文件再搜索，写在temp中！
	// 如何求或呢？每一条都搜索一遍temp，不存在就写到文件末尾！
	// 问题：块不连续了，可能会耗时长
	// who cares?? 我都用遍历求或了，怎么还会在乎块的事情
	// 而且根据空间局部性，那个块应该在物理内存里！
	int selectSingleConditionFirst(TableMeta meta, Condition condition);
	int selectSingleConditionAnd(TableMeta meta, Condition condition, int ptrNum);
	int selectSingleConditionOr(TableMeta meta, Condition condition, int ptrNum);
	int selectWithoutCondition(TableMeta meta);
	void recordFilePtr(Pointer ptr, int offset);
	void printWithPtrFile(TableMeta meta, int ptrNum);
	int selectNoPrint(TableMeta meta, Where where);
	bool isExistPtr(Pointer ptr, int ptrNum);
	bool isValueUnique(TableMeta meta, string attriName, Data value);
	Tuple getDeleteLastTuple(TableMeta meta, Where where, Pointer now, int &number);
	Tuple loadTuple(unsigned char *buffer, int &location, const vector<Attribute> &attributeVec);
	int deleteWithPointer(TableMeta meta, Pointer ptr);
	BufferManager BM, &TM;
	IndexManager &IM;
};

#endif
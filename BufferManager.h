//BufferManager.h
// 内存和磁盘的交互区

#define _CRT_SECURE_NO_WARNINGS
#ifndef BUFFERMANAGER_H
#define BUFFERMANAGER_H
#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstring>
#include "exception.h"

#define PAGESIZE 1024

using namespace std;
//文件头信息
struct fileInfo
{
	int blockNum; //总的块号
	int recordLength;//每条记录的长度（针对等长记录）
	int recordNum;//文件记录数
};
// Page类。缓冲区分页设置
class Page {
public:
	// 构造函数和一些存取控制函数。可忽略。
	Page();
	//恢复初始
	void initialize();
	//设置对应的块号
	void setBlockId(int blockId);
	//获取对应的块号
	int getBlockId();
	//设置一个页是否被钉住
	void setPin(bool isPin);
	//获取页面是否被钉住
	bool getPin();
	//设置页面脏位
	void setDirtyBit(bool dirty);
	//获取页面脏位
	bool getDirtyBit();
	//设置页面引用位 首次载入时和之后用到时都设置为true
	void setRefBit(bool ref);
	//获取页面引用位
	bool getRefBit();
	//获取对应的缓冲区
	unsigned char* getBuffer();
	friend class BufferManager;
private:
	unsigned char buffer[PAGESIZE];//每一页都是一个大小为PAGESIZE字节的数组
	string filename;//页所对应的文件名
	int blockId;//页在所在文件中的块号(磁盘中通常叫块)
	bool pin;//记录被钉住的次数。被钉住的意思就是不可以被替换
	bool dirtyBit;//dirty记录页是否被修改，当页发生删除或增加时需要设为真
	bool refBit;//ref变量表示此页面最近被引用过，首次载入和查找、增加、删除时均需使用
	int usedBit;//表明此页中已经使用的字节数

};

// BufferManager类。对外提供操作缓冲区的接口。
class BufferManager {
public:
	//  构造函数
	BufferManager(int initialSize=1024);
	// 析构函数
	~BufferManager();

	//核心函数 获得外存blockid中对应的frame页号 所有模块均需使用
	int getPageId(string fileName, int blockId);
	//获得对应的frame缓存区
	unsigned char* getBuffer(int frameId);
	// 更改页面dirtybit record、catlog 模块 insert/update时使用 
	void setDirty(int frameId);
	//设置某页被引用过 select/insert/update时均需使用
	void setReferenced(int frameId);
	// 钉住一个页 set pin=1
	void pinPage(int frameId);
	//解除一个页的锁定 初始化（reset）时使用
	void unpinPage(int frameId);
	//获得页面是否被钉住
	bool getPin(int frameId);
	//获得已用字节数
	int getUsedBit(int frameId);
	//设定已用字节数
	void setUsedBit(int frameId, int newLength);
	//将某页初始化
	void setFileInfoInitialized(string filename);
	//删除一个文件的所有页信息
	void deleteFrames(string filename);
	//在文件属性列表删除某页的信息
	void removeFileInfo(string fileName);
	//一个表用来维护所有文件的属性
	unordered_map<string, fileInfo> fileAttribute;
private:
	
	Page* frames;
	//一个哈希表用来维护（文件名，块号）——页号之间的关系
	unordered_map<string, unordered_map<int, int> >record;
	//读入新的block后增加相关信息
	void insertIntoHashMap(string filename, int blockId, int frameId);
	//flushpage后删除相应的信息
	void deleteFromHashMap(string filename, int blockId, int frameId);
	int maxSize;//记录总页数
	int currentPosition;//时钟替换策略需要用到的变量
	//将硬盘内的数据读入
	void loadDiskData(int frame_id, string file_name, int blockId);
	// 将对应内存页写入对应文件的对应块。
	void flushPage(int page_id, std::string file_name, int blockId);
	//结束时将全部缓冲区中数据写入,析构调用
	void flushAllPage();
	//将某页设为初始化
	void setInitialized(int frameId);
	//删除这个页及其所有相关信息
	void removeFrames(int frameId);
};



#endif


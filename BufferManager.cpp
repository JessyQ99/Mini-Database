#include"BufferManager.h"
Page::Page()
{
	initialize();
}

//缓冲区页面初始化
void Page::initialize()
{
	memset(buffer, 0, PAGESIZE);
	filename = "";
	blockId = -1;
	pin = false;
	dirtyBit = false;
	refBit = false;
	usedBit = 0;
}

//设置页面对应的blockid
void Page::setBlockId(int block_id)
{
	blockId = block_id;
}

//设置页面的脏位(dirtybit)
void Page::setDirtyBit(bool dirty_bit)
{
	dirtyBit = dirty_bit;
}

//设置页面是否被钉住
void Page::setPin(bool isPin)
{
	pin = isPin;
}
//设置页面的引用位
void Page::setRefBit(bool ref_bit)
{
	refBit = ref_bit;
}
//获取页面对应的块号
int Page::getBlockId()
{
	return blockId;
}
//获取页面的dirtyBit
bool Page::getDirtyBit()
{
	return dirtyBit;
}
//获取页面是否被钉住
bool Page::getPin()
{
	return pin;
}
//获取引用位
bool Page::getRefBit()
{
	return refBit;
}
//获取缓冲区页对应的文件名


//返回缓冲区字段
unsigned char*Page::getBuffer()
{
	return buffer;
}

//构造函数 分配缓冲区
BufferManager::BufferManager(int initialize_size)
{
	maxSize = initialize_size;
	frames = new Page[initialize_size];
	for (int i = 0; i < maxSize; i++)
		frames[i].initialize();
	currentPosition = 0;
}
//析构函数 释放缓冲区
BufferManager::~BufferManager()
{
	FILE*f;
	//先将更新的文件头信息写入
	for (auto it = fileAttribute.begin(); it != fileAttribute.end(); it++)
	{
		f = fopen((*it).first.c_str(), "r+");
		if(f != NULL)
			// throw FileIO_error();
			fwrite(&(*it).second, sizeof(fileInfo), 1, f);
	}
	//将更新的block写入
	flushAllPage();
	delete []frames;
}


//核心函数 获取文件块号对应的缓冲区页面编号
int BufferManager::getPageId(string filename, int blockId)
{
	int i,usedBit;
	//查找是否存在相应的记录
	if (record.find(filename) != record.end())
	{
		if (record[filename].find(blockId) != record[filename].end())
		{
			return record[filename][blockId];
		}
	}
	//如果没有相应的记录则置换出一页空页
	while (1)
	{
		//第一遍扫描 寻找引用位位0且脏页位0的页面
		for (i = 0; i < maxSize; currentPosition++, i++)
		{
			currentPosition = currentPosition % maxSize;
			
			//如果页面没有被钉住，且页面没有被引用而且页面不是脏页
			if (!frames[currentPosition].pin && !frames[currentPosition].refBit && !frames[currentPosition].dirtyBit)
			{
				deleteFromHashMap(filename, frames[currentPosition].blockId, currentPosition);
				//因为页面不是脏页所以无需重写回磁盘，只需再次初始化页面就行
				frames[currentPosition].initialize();
				//设定次页面相应的属性
				frames[currentPosition].filename = filename;
				frames[currentPosition].blockId = blockId;
				//从磁盘加载进对应的文件块
				loadDiskData(currentPosition, filename, blockId);
				//时钟指针指向下一个位置返回当前位置
				
				return currentPosition++;
			}
			
		}
		//第二遍扫描寻找引用位为0且脏页为1的页面
		for (i = 0; i < maxSize; currentPosition++, i++)
		{
			currentPosition = currentPosition % maxSize;
			if (!frames[currentPosition].pin && !frames[currentPosition].refBit)
			{
				//对于脏页在置换之前需要将其写回磁盘
				flushPage(currentPosition, frames[currentPosition].filename, frames[currentPosition].blockId);
				frames[currentPosition].filename = filename;
				frames[currentPosition].blockId = blockId;
				loadDiskData(currentPosition, filename, blockId);
				
				return currentPosition++;
			}
			//此遍扫描需把引用位都置为0
			frames[currentPosition].refBit = false;
		}
	}
}

//获取页号对应的缓冲区字段
unsigned char*BufferManager::getBuffer(int frameId)
{

	//对每次引用的页面refbit都设为1
	frames[frameId].refBit = true;
	return frames[frameId].getBuffer();
}

//从磁盘读取相应数据
void BufferManager::loadDiskData(int frameId, string filename, int blockId)
{
	
	FILE* f = fopen(filename.c_str(), "r");
	if (f == NULL)
	{
		cout << "loadDisk:" << endl;
		cout << filename << ":ERROR" << endl;
		cout << "Block id: " << blockId << endl;
		throw FileIO_error();
	}
	
	//读入文件头信息
	fileInfo info;
	fread(&info, sizeof(fileInfo), 1, f);
	if(fileAttribute.find(filename) == fileAttribute.end())
		fileAttribute[filename] = info;
	
	//通过seek指针定位至文件对应的block区，注意文件头的偏移
	fseek(f, PAGESIZE * blockId+sizeof(fileInfo), SEEK_SET);
	unsigned char* buffer = frames[frameId].getBuffer();
	//读入相应的block
	fread(buffer, PAGESIZE, 1, f);
	int usedBit = *(int*)(string(buffer, buffer + 4).c_str());
	//更新块使用字节的记录
	frames[frameId].usedBit = usedBit;
	//首次加载进磁盘引用位为1
	frames[frameId].setRefBit(true);
	
	fclose(f);
	//在（块号，页号）中添加相应记录
	insertIntoHashMap(filename, blockId, frameId);

	return;

}

// 核心函数之一。内存和磁盘交互的接口。
void BufferManager::flushPage(int frameId, std::string filename, int blockId)
{
	// 打开文件
	FILE* f = fopen(filename.c_str(), "r+");
	if (!f) {
		cout << "flushDisk ERROR:" << endl;
		cout << filename << ":ERROR" << endl;
		cout << "frame Id: " << frameId << endl;
		cout << "Block id: " << blockId << endl;
		cout << "BlockNum: " << fileAttribute["./record/temp"].blockNum << endl;
		throw FileIO_error();
	}
	//定位到block中相应的位置 注意文件头信息的偏移
	fseek(f, PAGESIZE * blockId+sizeof(fileInfo), SEEK_SET);
	unsigned char* buffer = frames[frameId].getBuffer();
	int usedBit = frames[frameId].usedBit;
	//在存入磁盘之前别忘更新相应的记录已用字节的大小
	memcpy(buffer, &usedBit, sizeof(int));
	//写入相应的磁盘文件
	fwrite(buffer, PAGESIZE, 1, f);
	fclose(f);
	//写入磁盘后缓冲区更新
	frames[frameId].initialize();
	//删除相应的位置记录
	deleteFromHashMap(filename, blockId, frameId);

}
//程序结束后 将所有缓冲区中的页面更新至磁盘
void BufferManager::flushAllPage()
{
	for (int i = 0; i < maxSize; i++)
		if(frames[i].blockId!=-1) {
			flushPage(i, frames[i].filename, frames[i].blockId);
		}
}
//在(blockid,frameid)中添加相应记录
void BufferManager::insertIntoHashMap(string filename, int blockId, int frameId)
{
	record[filename][blockId] = frameId;
}

//在(blockid,frameid)中删除相应的记录
void BufferManager::deleteFromHashMap(string filename, int blockId, int frameId)
{
	auto it = record[filename].find(blockId);
	if (it != record[filename].end())
		record[filename].erase(it);
	if (record[filename].size() == 0)
	{
		auto it2 = record.find(filename);
		record.erase(it2);
	}
}
//设置某页为脏页
void BufferManager::setDirty(int frameId)
{
	frames[frameId].dirtyBit = true;
}
//设置某页被引用过
void BufferManager::setReferenced(int frameId)
{
	frames[frameId].refBit = true;
}
//钉住某页
void BufferManager::pinPage(int frameId)
{
	frames[frameId].pin = true;
}
//解除此块的钉住状态
void BufferManager::unpinPage(int frameId)
{
	frames[frameId].pin = false;
}
//获取此块是否被钉住
bool BufferManager::getPin(int frameId)
{
	return frames[frameId].pin;
}

//获取已经使用的字节数
int BufferManager::getUsedBit(int frameId)
{
	
	return frames[frameId].usedBit;
}
//更新已经使用的字节数
void BufferManager::setUsedBit(int frameId, int newLength)
{
	frames[frameId].usedBit = newLength;
}
//如果要删除此block，最后将此block进行初始化操作
void BufferManager::setInitialized(int frameId)
{
	frames[frameId].initialize();
}

void BufferManager::removeFileInfo(string fileName)
{
	auto it = fileAttribute.find(fileName);
	if(it != fileAttribute.end()) 
		fileAttribute.erase(it);
}

void BufferManager::removeFrames(int frameId)
{
	setInitialized(frameId);
	deleteFromHashMap(frames[frameId].filename, frames[frameId].blockId, frameId);
	removeFileInfo(frames[frameId].filename);
}

void BufferManager::setFileInfoInitialized(string filename)
{
	fileAttribute[filename].blockNum=0;
	fileAttribute[filename].recordLength=0;
	fileAttribute[filename].recordNum=0;
}


void BufferManager::deleteFrames(string filename)
{


	for(auto i : record[filename])
		removeFrames(i.second);
}
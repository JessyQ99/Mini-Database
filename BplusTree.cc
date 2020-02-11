#include "BplusTree.h"

// offset指的是第几条记录
void recordFilePtrForIndex(Pointer ptr, int offset, BufferManager &TM)
{
    string filePath = "./record/temp";

    // 检测文件是否存在， 不存在创建文件
    FILE *fp = fopen(filePath.c_str(), "r");
    if(fp == NULL) {
        fp = fopen(filePath.c_str(), "w");
    }
    fclose(fp);

    const int maxPtrInBlock = (PAGESIZE - 4) / sizeof(Pointer);
    int blockID = offset / maxPtrInBlock;

    // 获取buffer中对应相应块号的页编号
    int frameId = TM.getPageId(filePath, blockID);
    unsigned char *buffer = TM.getBuffer(frameId);
    // 需要增加一个块了
    if (blockID > TM.fileAttribute[filePath].blockNum-1){
        TM.fileAttribute[filePath].blockNum++;
    }

    int offsetInBlock = (offset % maxPtrInBlock) * sizeof(Pointer) + 4;

    memcpy(buffer + offsetInBlock, &ptr, sizeof(Pointer));
    TM.fileAttribute[filePath].recordNum++;
    TM.setDirty(frameId);
    TM.setReferenced(frameId);
}
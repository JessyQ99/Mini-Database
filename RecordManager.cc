#include "RecordManager.h"
#include <sstream>
#include <set>
// block id, no

RecordManager::~RecordManager()
{
    const string filePath = "./record/temp";
    TM.deleteFrames(filePath);
    remove(filePath.c_str());
}

int RecordManager::getRecordLength(TableMeta meta)
{
    int recordLength = 0;
	for(auto i: meta.attributeVec) {
        if(i.type == 0)
            recordLength += sizeof(int);
        else if(i.type < 0)
            recordLength += sizeof(float);
        else
            recordLength += (i.type + 1) * sizeof(char);
    }
    return recordLength;
}

int RecordManager::dropTable(string tableName)
{
    // 根据usedbit计算有多少个record
    string filePath = "./record/" + tableName;
    BM.removeFileInfo(filePath);
    BM.deleteFrames(filePath);
    remove(filePath.c_str());

    return 0;
}

bool isConditionOk(TableMeta meta, Where where, Tuple tuple)
{
    
    if(where.conditionVec.empty())
        return true;
    
    vector<bool> result;
    for(auto condition : where.conditionVec) {
        int index = 0;
        for(auto attribute : meta.attributeVec) {
            if(attribute.name == condition.attribute)
                break;
            index++;
        }
        switch (condition.relation)
        {
        case EQUAL:
            result.push_back(tuple[index] == condition.value);
            break;
        case NOT_EQUAL:
            result.push_back(tuple[index] != condition.value);
            break;
        case LESS:
            result.push_back(tuple[index] < condition.value);
            break;
        case LESS_EQUAL:
            result.push_back(tuple[index] <= condition.value);
            break;
        case LARGE:
            result.push_back(tuple[index] > condition.value);
            break;    
        case LARGE_EQUAL:
            result.push_back(tuple[index] >= condition.value);
            break;                                                
        default:
            throw Relation_Not_Exist_Error();
            break;
        }
    }

    bool finalResult = result[0];
    int no = 1;
    for(auto logic : where.opVec) {
        switch (logic)
        {
        case AND:
            finalResult &= result[no++];
            break;
        case OR:
            finalResult |= result[no++];
            break;      
        default:
            break;
        }
    }
    return finalResult;
}

int RecordManager::deleteWithPointer(TableMeta meta, Pointer ptr)
{
    const string tablePath = "./record/" + meta.tableName;

    // 用于暂时解决Buffer 的bug
    BM.getPageId(tablePath, 0);
    // 打开最后一条记录所在的块
    int blockId = BM.fileAttribute[tablePath].blockNum - 1;
    int pageId = BM.getPageId(tablePath, blockId);
    unsigned char *buffer = BM.getBuffer(pageId);
    // 得到最后一条记录所在块使用的字节数
    int usedBit = BM.getUsedBit(pageId);
    int recordLength = getRecordLength(meta);
    // 指向最后一条记录的位置
    int offset = usedBit - recordLength + 4;
    Pointer ptrToMove(blockId, offset);
    // 如果与现在的位置相同，证明所要删除的记录是最后
    if(ptrToMove == ptr) {
        if(ptr.offset <= 4)
            --BM.fileAttribute[tablePath].blockNum;
        // 清空条件
        BM.setDirty(pageId);
        BM.setReferenced(pageId);
        BM.setUsedBit(pageId, offset - 4);
        return 1;
    }
    Tuple tuple = loadTuple(buffer, offset, meta.attributeVec);
    // 因为loadTuple会把offset前移一个记录的身位，所以要减回去
    offset -= recordLength;

    int srcId = BM.getPageId(tablePath, ptr.blockID);
    unsigned char *srcBuffer = BM.getBuffer(srcId);
    // 将Tuple存入删除位置
    int attriOffset = 0;
    for(auto data : tuple) {
        if(data.type == 0) {
            memcpy(srcBuffer + ptr.offset + attriOffset, &data.datai, sizeof(int));
            attriOffset += sizeof(int);
        }
        else if(data.type < 0) {
            memcpy(srcBuffer + ptr.offset + attriOffset, &data.dataf, sizeof(float));
            attriOffset += sizeof(float);
        }
        else {
            memcpy(srcBuffer + ptr.offset + attriOffset, data.datas.content.c_str(), (data.type + 1) * sizeof(char));
            attriOffset += sizeof(char) * (data.type + 1);
        }
    }
    // 该块已空
    if(offset <= 4) {
        // 这个文件的block数量减一
        --BM.fileAttribute[tablePath].blockNum;
        if(blockId < 0) {
            cout << "Delete Pointer Error" << endl;
            throw Block_Empty_Error();
        }
    }
    BM.setDirty(srcId);
    BM.setReferenced(srcId);
    BM.setDirty(pageId);
    BM.setReferenced(pageId);
    BM.setUsedBit(pageId, offset - 4);
    return 0;
}

int RecordManager::deLete(TableMeta meta, Where where)
{
    const string tablePath = "./record/" + meta.tableName;
    const int recordLength = getRecordLength(meta);
    int number = 0;

    set<string> indexSet;
    for(auto att : meta.attributeVec)
        if(att.isIndex)
            indexSet.insert(att.name);
    int indexOffset = 0;
    for(auto cond : where.conditionVec) {
        if(indexSet.find(cond.attribute) != indexSet.cend())
            break;
        else 
            indexOffset++;
    }

    BM.getPageId(tablePath, 0);
    for (int blockId = 0; blockId < BM.fileAttribute[tablePath].blockNum; blockId++)
    {
        int frameId = BM.getPageId(tablePath, blockId);
        BM.pinPage(frameId);
        unsigned char *buffer = BM.getBuffer(frameId);
        // 跳过最开头的记录已使用字节数的整型变量
        int length = BM.getUsedBit(frameId);

        int location = 4;
        while(location - 4 < length) {
            Pointer now(blockId, location);
            Tuple tuple = loadTuple(buffer, location, meta.attributeVec);
            while(isConditionOk(meta, where, tuple)) {
                int isOver = deleteWithPointer(meta, now);
                number++;
                location -= recordLength;
                int i = 0;
                Tuple nowTuple = loadTuple(buffer, location, meta.attributeVec);
                location -= recordLength;
                for(auto Attri : meta.attributeVec) {
                    // 把有索引的都删了
                    if(Attri.isIndex) {
                        IM.deleteKey(meta.tableName, Attri.name, tuple[i]);
                        // 不是最后一个的话，还要把调换过来的更新一下
                        if(!isOver) {
                            IM.updateKey(meta.tableName, Attri.name, nowTuple[i], now);
                        }
                    }
                    i++;
                }
                // 如果删除的是最后一条记录，或者一页删完了
                if(isOver || location - 4 >= BM.getUsedBit(frameId))
                    break;
                location += recordLength;
                tuple = nowTuple;
            }
            length = BM.getUsedBit(frameId);
        }
        BM.unpinPage(frameId);
    }
    return number;
}

int RecordManager::createTable(TableMeta meta)
{
    string filePath = "./record/" + meta.tableName;
    FILE *fp = fopen(filePath.c_str(), "w");
    fclose(fp);
    BM.setFileInfoInitialized(filePath);

    return 0;
}

bool RecordManager::isValueUnique(TableMeta meta, string attriName, Data value)
{
    Condition condition(attriName, value, EQUAL);
    return selectSingleConditionFirst(meta, condition) == 0;
}

int RecordManager::insert(TableMeta meta, Tuple tuple)
{
    // 唯一性检测
    int index = 0;
    for(auto attribute : meta.attributeVec) {
        if(attribute.isUnique && !isValueUnique(meta, attribute.name, tuple[index])) {
            bool isPrimary = false;
            for(auto i : meta.primaryKeyVec) {
                if(i == attribute.name)
                    isPrimary = true;
            }
            string value, attributeName;
            if(isPrimary)
                attributeName = "PRIMARY";
            else
                attributeName = attribute.name;
            stringstream ss;
            if(tuple[index].type == 0) {
                ss << tuple[index].datai;
                ss >> value;
            }
            else if(tuple[index].type < 0) {
                ss << tuple[index].dataf;
                ss >> value;
            }
            else {
                value = string(tuple[index].datas.content);
            }
            throw Duplicate_Error(attributeName, value);
        }
        index++;
    }
	// 计算表的记录长度
    int recordLength = getRecordLength(meta);
    string filePath = "./record/" + meta.tableName;

    BM.getPageId(filePath, 0);
    if(BM.fileAttribute[filePath].blockNum == 0)
        BM.fileAttribute[filePath].blockNum++;
    int blockId = BM.fileAttribute[filePath].blockNum - 1;
    // 获取buffer中对应相应块号的页编号
    int frameId = BM.getPageId(filePath, blockId);
    unsigned  char *buffer = BM.getBuffer(frameId);
    // 跳过最开头的记录已使用字节数的整型变量
    int length = BM.getUsedBit(frameId);
    // 记录太长无法写入相应的block 
    // 前4个字节记录存放记录的大小
    // 使用新块
    if (length + recordLength > PAGESIZE - 4) {
        blockId = ++BM.fileAttribute[filePath].blockNum - 1;
        // 获取buffer中对应相应块号的页编号
        frameId = BM.getPageId(filePath, blockId);
        buffer = BM.getBuffer(frameId);
        // 跳过最开头的记录已使用字节数的整型变量
        length = BM.getUsedBit(frameId);
    }
    //增加一条新的记录
    BM.fileAttribute[filePath].recordNum++;
    BM.setUsedBit(frameId, recordLength + length);
    int location = length + 4;
    Pointer pointer(blockId, location);
    for(auto i : tuple) {
        if(i.type == 0) {
            memcpy(buffer + location, &i.datai, sizeof(int));
            location += sizeof(int);
        }
        else if(i.type < 0) {
            memcpy(buffer + location, &i.dataf, sizeof(float));
            location += sizeof(float);
        }
        else {
            memcpy(buffer + location, i.datas.content.c_str(), (i.type + 1) * sizeof(char));
            location += (i.type + 1) * sizeof(char);
        }
    }
    //设置此页面刚刚被引用过
    BM.setReferenced(frameId);
    //修改过页面页面设置为脏页
    BM.setDirty(frameId);

    int i = 0;
    for(auto Attr : meta.attributeVec) {
        if(Attr.isIndex) {
            IM.insertKey(meta.tableName, Attr.name, tuple[i], pointer);
        }
        i++;
    }
    return 0;
}

int RecordManager::selectNoPrint(TableMeta meta, Where where)
{
    if(where.conditionVec.empty()) {
        return selectWithoutCondition(meta);
    }
    else
    {
        int result = selectSingleConditionFirst(meta, where.conditionVec[0]);
        int conditionNo = 1;
        for(auto op : where.opVec) {
            switch (op)
            {
            case AND:
                result = selectSingleConditionAnd(meta, where.conditionVec[conditionNo++], result);
                break;
            case OR:
                result = selectSingleConditionOr(meta, where.conditionVec[conditionNo++], result);
                break;
            default:
                throw Logic_Not_Exist_Error();
                break;
            }
        }
        return result;
    }  
}

int RecordManager::select(TableMeta meta, Where where)
{
    int resultNum = selectNoPrint(meta, where);
    vector<int> strSize;
    for(auto attri : meta.attributeVec) {
        strSize.push_back(attri.name.length());
    }
    cout << "+";
    for(auto i : strSize) {
        for (int j = 0; j < i + 2; j++)
            cout << "-";
        cout << "+";
    }
    cout << endl;
    cout << "| ";
    for( auto name : meta.attributeVec) {
        cout << name.name << " | ";
    }
    cout << endl;
    cout << "+";
    for(auto i : strSize) {
        for (int j = 0; j < i + 2; j++)
            cout << "-";
        cout << "+";
    }
    cout << endl;
    printWithPtrFile(meta, resultNum);

    return resultNum;
}

// 应该返回一个文件指针文件
int RecordManager::selectWithoutCondition(TableMeta meta)
{
    int recordLength = getRecordLength(meta);
    string filePath = "./record/" + meta.tableName;

    int recordNo = 0;
    
    BM.getPageId(filePath, 0);
	// 遍历所有的block寻找合适的能存储记录blockid
	for (int blockId = 0; blockId < BM.fileAttribute[filePath].blockNum; blockId++)
	{
        // 获取buffer中对应相应块号的页编号
		int frameId = BM.getPageId(filePath, blockId);
		unsigned char *buffer = BM.getBuffer(frameId);
		// 跳过最开头的记录已使用字节数的整型变量
		int length = BM.getUsedBit(frameId);

        int location = 4;
        while(location - 4 < length) {
            Tuple tuple;
            Pointer ptr(blockId, location);
            tuple = loadTuple(buffer, location, meta.attributeVec);
            recordFilePtr(ptr, recordNo++);
        }
        BM.setReferenced(frameId);
    }

    return recordNo;
}

int RecordManager::selectSingleConditionFirst(TableMeta meta, Condition condition)
{
    int recordLength = getRecordLength(meta);
    string filePath = "./record/" + meta.tableName;

    
    int offset = 0;
    for(auto i : meta.attributeVec) {
        if(i.name == condition.attribute)
            break;
        else
            offset++;
    }
    if(offset == meta.attributeVec.size())
        throw Attribute_Not_Exist_Error(condition.attribute);
    
    int recordNo = 0;
    if(meta.attributeVec[offset].isIndex) {
        recordNo = IM.SearchRetFile(meta.tableName, condition);
    }
    else { 
        // 遍历所有的block寻找合适的能存储记录blockid
        BM.getPageId(filePath, 0);
        for (int blockId = 0; blockId < BM.fileAttribute[filePath].blockNum; blockId++)
        {
            // 获取buffer中对应相应块号的页编号
            int frameId = BM.getPageId(filePath, blockId);
            unsigned char *buffer = BM.getBuffer(frameId);
            // 跳过最开头的记录已使用字节数的整型变量
            int length = BM.getUsedBit(frameId);

            int location = 4;
            while(location - 4 < length) {
                Tuple tuple;
                Pointer ptr(blockId, location);
                tuple = loadTuple(buffer, location, meta.attributeVec);
                switch (condition.relation)
                {
                case EQUAL:
                    if(tuple[offset] == condition.value) {
                        recordFilePtr(ptr, recordNo++);
                    }
                    break;
                case NOT_EQUAL:
                    if(tuple[offset] != condition.value) {
                        recordFilePtr(ptr, recordNo++);
                    }
                    break;
                case LESS:
                    if(tuple[offset] < condition.value) {
                        recordFilePtr(ptr, recordNo++);
                    }
                    break;
                case LESS_EQUAL:
                    if(tuple[offset] <= condition.value) {
                        recordFilePtr(ptr, recordNo++);
                    }
                    break;
                case LARGE:
                    if(tuple[offset] > condition.value) {
                        recordFilePtr(ptr, recordNo++);
                    }
                    break;    
                case LARGE_EQUAL:
                    if(tuple[offset] >= condition.value) {
                        recordFilePtr(ptr, recordNo++);
                    }
                    break;                                                
                default:
                    throw Relation_Not_Exist_Error();
                    break;
                }
            }

            BM.setReferenced(frameId);
        }
    }
    return recordNo;
}

// 思路：
// 需要做到：增加一个指针
//          从头开始写
// 也就是说，需要指定写入位置
// 外部函数需要自己记录写到第几条了

// offset指的是第几条记录
void RecordManager::recordFilePtr(Pointer ptr, int offset)
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

int RecordManager::selectSingleConditionAnd(TableMeta meta, Condition condition, int ptrNum)
{
    int recordLength = getRecordLength(meta);
    string filePath = "./record/temp";
    string tablePath = "./record/" + meta.tableName;

    // 获得目标属性是第几个属性
    int AttriNo = 0;
    for(auto i : meta.attributeVec) {
        if(i.name == condition.attribute)
            break;
        else {
            AttriNo++;
        }
    }
    if(AttriNo == meta.attributeVec.size())
        throw Attribute_Not_Exist_Error(condition.attribute);

    int ptrNo = 0;
    int recordNo = 0;
    TM.getPageId(filePath, 0);
    // 对temp文件进行一次线性扫描，寻找符合条件的指针
    for(int blockId = 0; ptrNo < ptrNum && blockId < TM.fileAttribute[filePath].blockNum; blockId++) {
        int frameId = TM.getPageId(filePath, blockId);
        unsigned char *buffer = TM.getBuffer(frameId);
        const int maxPtrInBlock = (PAGESIZE - 4) / sizeof(Pointer);

        int ptrInThisBlock = 0;
        while(ptrNo < ptrNum && ptrInThisBlock < maxPtrInBlock) {
            Pointer ptr = loadPtr(buffer, ptrInThisBlock++);
            Pointer ptrToStore = ptr;
            ptrNo++;
            int valueFrame = BM.getPageId(tablePath, ptr.blockID);
            unsigned char *valueBuffer = BM.getBuffer(valueFrame);
            Tuple tuple = loadTuple(valueBuffer, ptr.offset, meta.attributeVec);
            switch (condition.relation)
            {
            case EQUAL:
                if(tuple[AttriNo] == condition.value) {
                    recordFilePtr(ptrToStore, recordNo++);
                }
                break;
            case NOT_EQUAL:
                if(tuple[AttriNo] != condition.value) {
                    recordFilePtr(ptrToStore, recordNo++);
                }
                break;
            case LESS:
                if(tuple[AttriNo] < condition.value) {
                    recordFilePtr(ptrToStore, recordNo++);
                }
                break;
            case LESS_EQUAL:
                if(tuple[AttriNo] <= condition.value) {
                    recordFilePtr(ptrToStore, recordNo++);
                }
                break;
            case LARGE:
                if(tuple[AttriNo] > condition.value) {
                    recordFilePtr(ptrToStore, recordNo++);
                }
                break;    
            case LARGE_EQUAL:
                if(tuple[AttriNo] >= condition.value) {
                    recordFilePtr(ptrToStore, recordNo++);
                }
                break;                                                
            default:
                throw Relation_Not_Exist_Error();
                break;
            }
        }
        BM.setReferenced(frameId);
    }

    return recordNo;
}

int RecordManager::selectSingleConditionOr(TableMeta meta, Condition condition, int ptrNum)
{
	// 计算表的记录长度
    // 约定 type = 0 为整形
    //     type < 0 为float
    //     type > 0 为字符串，数值为字符串的长度
    int recordLength = getRecordLength(meta);
    string filePath = "./record/" + meta.tableName;

    int offset = 0;
    for(auto i : meta.attributeVec) {
        if(i.name == condition.attribute)
            break;
        else {
            offset++;
        }
    }
    if(offset == meta.attributeVec.size())
        throw Attribute_Not_Exist_Error(condition.attribute);

    int recordNo = ptrNum;

    // 线性搜索
    for (int blockId = 0; blockId < BM.fileAttribute[filePath].blockNum; blockId++)
    {
        // 获取buffer中对应相应块号的页编号
        int frameId = BM.getPageId(filePath, blockId);
        unsigned char *buffer = BM.getBuffer(frameId);
        // 跳过最开头的记录已使用字节数的整型变量
        int length = BM.getUsedBit(frameId);

        int location = 4;
        while(location - 4 < length) {
            Tuple tuple;
            Pointer ptr(blockId, location);
            tuple = loadTuple(buffer, location, meta.attributeVec);
            switch (condition.relation)
            {
            case EQUAL:
                if(tuple[offset] == condition.value && !isExistPtr(ptr, ptrNum)) {
                    recordFilePtr(ptr, recordNo++);
                }
                break;
            case NOT_EQUAL:
                if(tuple[offset] != condition.value && !isExistPtr(ptr, ptrNum)) {
                    recordFilePtr(ptr, recordNo++);
                }
                break;
            case LESS:
                if(tuple[offset] < condition.value && !isExistPtr(ptr, ptrNum)) {
                    recordFilePtr(ptr, recordNo++);
                }
                break;
            case LESS_EQUAL:
                if(tuple[offset] <= condition.value && !isExistPtr(ptr, ptrNum)) {
                    recordFilePtr(ptr, recordNo++);
                }
                break;
            case LARGE:
                if(tuple[offset] > condition.value && !isExistPtr(ptr, ptrNum)) {
                    recordFilePtr(ptr, recordNo++);
                }
                break;    
            case LARGE_EQUAL:
                if(tuple[offset] >= condition.value && !isExistPtr(ptr, ptrNum)) {
                    recordFilePtr(ptr, recordNo++);
                }
                break;                                                
            default:
                throw Relation_Not_Exist_Error();
                break;
            }
        }
        BM.setDirty(frameId);
    }
    return recordNo;
}

Tuple RecordManager::loadTuple(unsigned char *buffer, int &location, const vector<Attribute> &attributeVec)
{
    Tuple tuple;
    for(auto i : attributeVec) {
        if(i.type == 0) {
            int datai;
            memcpy(&datai, buffer + location, sizeof(int));
            Data data;
            data.datai = datai;
            data.type = 0;
            tuple.push_back(data);
            location += sizeof(int);
        }
        else if(i.type < 0) {
            float dataf;
            memcpy(&dataf, buffer + location, sizeof(float));
            Data data;
            data.dataf = dataf;
            data.type = -1;
            tuple.push_back(data);
            location += sizeof(float);
        }
        else {
            char *str = new char[i.type + 1];
            memcpy(str, buffer + location, i.type + 1);
            Data data;
            Varchar var;
            var.content = string(str);
            var.maxSize = i.type;
            data.datas = var;
            data.type = i.type;
            tuple.push_back(data);
            location += i.type + 1;
            delete[] str;
        }
    }
    return tuple;
}

Pointer loadPtr(unsigned char *buffer, int ptrNo)
{
    Pointer ptr;
    memcpy(&ptr, buffer + ptrNo * sizeof(Pointer) + 4, sizeof(Pointer));
    return ptr;
}

void RecordManager::printWithPtrFile(TableMeta meta, int ptrNum)
{
    const string filePath = "./record/temp";
    const string tablePath = "./record/" + meta.tableName;
    
    int ptrNo = 0;
    TM.getPageId(filePath, 0);
    for(int blockId = 0; ptrNo < ptrNum && blockId < TM.fileAttribute[filePath].blockNum; blockId++) {
        int frameId = TM.getPageId(filePath, blockId);
        unsigned char *buffer = TM.getBuffer(frameId);
        const int maxPtrInBlock = (PAGESIZE - 4) / sizeof(Pointer);

        int ptrInThisBlock = 0;
        while(ptrNo < ptrNum && ptrInThisBlock < maxPtrInBlock) {
            Pointer ptr = loadPtr(buffer, ptrInThisBlock++);
            ptrNo++;
            int valueFrame = BM.getPageId(tablePath, ptr.blockID);
            unsigned char *valueBuffer = BM.getBuffer(valueFrame);
            Tuple tuple = loadTuple(valueBuffer, ptr.offset, meta.attributeVec);
            
            for(auto i : tuple) {
                cout << "| ";
                if(i.type == 0)
                    cout << i.datai;
                else if(i.type < 0)
                    cout << i.dataf;
                else
                    cout << i.datas.content;
                cout << " ";
            }
            cout << "|" << endl;
            cout << "+";
            for(auto i : meta.attributeVec) {
                for (int j = 0; j < i.name.length() + 2; j++)
                    cout << "-";
                cout << "+";
            }
            cout << endl;
        }
    }
}

bool RecordManager::isExistPtr(Pointer ptr, int ptrNum)
{
    const string filePath = "./record/temp";
    
    int ptrNo = 0;
    TM.getPageId(filePath, 0);
    for(int blockId = 0; ptrNo < ptrNum && blockId < TM.fileAttribute[filePath].blockNum; blockId++) {
        int frameId = TM.getPageId(filePath, blockId);
        unsigned char *buffer = TM.getBuffer(frameId);
        const int maxPtrInBlock = (PAGESIZE - 4) / sizeof(Pointer);

        int ptrInThisBlock = 0;
        while(ptrNo < ptrNum && ptrInThisBlock < maxPtrInBlock) {
            Pointer ptrInFile = loadPtr(buffer, ptrInThisBlock++);
            ptrNo++;
            if(ptr == ptrInFile)
                return true;
        }
    }

    return false;
}

int RecordManager::createIndex(TableMeta meta, string indexAttr)
{
    int offset = 0;
    for(auto i : meta.attributeVec) {
        if(i.name == indexAttr)
            break;
        offset++;
    }
    if(offset == meta.attributeVec.size()) {
        cout << "createIndex ERROR" << endl;
        throw Block_Empty_Error();
    }

    int recordNum = 0;
    string filePath = "./record/" + meta.tableName;
    BM.getPageId(filePath, 0);
    for(int blockId = 0; blockId < BM.fileAttribute[filePath.c_str()].blockNum; blockId++) {
        int pageId = BM.getPageId(filePath, blockId);
        unsigned char *buffer = BM.getBuffer(pageId);
        int length = BM.getUsedBit(pageId);
        int location = 4;
        while(location < length + 4) {
            Pointer ptr(blockId, location);
            Tuple tuple = loadTuple(buffer, location, meta.attributeVec);
            IM.insertKey(meta.tableName, indexAttr, tuple[offset], ptr);
            recordNum++;
        }
    }

    return recordNum;
}
#include "IndexManager.h"

// 开启所有的B+树
IndexManager::IndexManager(BufferManager &TM):intTree(sizeof(int)), floatTree(sizeof(float)), stringTree(sizeof(IndexString)), TM(TM){
    ifstream infile;
    bplus_tree_config in_conf;
    string temp;
    infile.open(_conf_path);

    while(getline(infile, temp)){
        if(temp.empty()){
            continue;
        }
        memcpy(in_conf.filename, temp.c_str(), 1024);
		getline(infile, temp);
		in_conf.type = stoi(temp);
		getline(infile, in_conf.tableName);
		getline(infile, in_conf.attribute);
        getline(infile, in_conf.indexName);
        treeList.insert(pair<string, bplus_tree_config>(in_conf.indexName, in_conf));
    }
    infile.close();

    for(auto tree : treeList){
        bplus_tree *tempTree;
        if(tree.second.type == 0){
            tempTree = intTree.bplus_tree_init(tree.second.filename, _block_size);
        }
        else if(tree.second.type < 0){
            tempTree = floatTree.bplus_tree_init(tree.second.filename, _block_size);
        }
        else if(tree.second.type > 0){
            tempTree = stringTree.bplus_tree_init(tree.second.filename, _block_size);
        }
        activeTreeList.insert(pair<string, bplus_tree *>(tree.second.indexName, tempTree));
    }
};

// 关闭所有的B+树
IndexManager::~IndexManager()
{
    ofstream outfile;
    outfile.open(_conf_path);

    for(auto tree : treeList){
        outfile << tree.second.filename << endl;
        outfile << tree.second.type << endl;
        outfile << tree.second.tableName << endl;
        outfile << tree.second.attribute << endl;
        outfile << tree.second.indexName << endl;
        if(tree.second.type == 0){
            intTree.bplus_tree_deinit(activeTreeList[tree.first]);
        }
        else if(tree.second.type < 0){
            floatTree.bplus_tree_deinit(activeTreeList[tree.first]);
        }
        else if(tree.second.type > 0){
            stringTree.bplus_tree_deinit(activeTreeList[tree.first]);
        }
    }
    outfile.close();
}

// 创建Index
void IndexManager::createIndex(Index _index, TYPE type)
{
    string file_name = "./index/" + _index.indexName + ".index";
    bplus_tree_config config;
    bplus_tree *tempTree;

    config.type = type;
    strcpy(config.filename, file_name.c_str());
    config.attribute = _index.attribute;
    config.tableName = _index.tableName;
    config.indexName = _index.indexName;
    treeList.insert(pair<string, bplus_tree_config>(_index.indexName, config));

    if(type == 0){
        tempTree = intTree.bplus_tree_init(config.filename, _block_size);
    }
    else if(type < 0){
        tempTree = floatTree.bplus_tree_init(config.filename, _block_size);
    }
    else if(type > 0){
        tempTree = stringTree.bplus_tree_init(config.filename, _block_size);
    }
    activeTreeList.insert(pair<string, bplus_tree*>(_index.indexName, tempTree));
}

// 删除index
void IndexManager::dropIndex(string indexName)
{
    string file_path;
    string boot_path;

    file_path = "./index/" + indexName + ".index";
    boot_path = file_path + ".boot";

    free(activeTreeList[indexName]->caches);
    free(activeTreeList[indexName]);
    activeTreeList.erase(indexName);
    treeList.erase(indexName);
    remove(file_path.c_str());
    remove(boot_path.c_str());
}

// 条件搜索，返回的结果保存在一个文件中
int IndexManager::SearchRetFile(string tableName, Condition condition)
{
    int result_num = 0;
    // cout << "Ret" << endl;
    long long data = 0;
    string indexName;
    bplus_tree *tempTree;
    for(auto it : treeList){
        if(it.second.tableName == tableName && it.second.attribute == condition.attribute){
            indexName = it.first;
            tempTree = activeTreeList[indexName];
            // cout <<  "tempTree: " << tempTree << endl;
        }
    }
    if(condition.value.type == 0){
        result_num = int_get_range(tempTree, condition);
    }
    else if (condition.value.type  < 0){
        result_num = float_get_range(tempTree, condition);
    }
    else if(condition.value.type > 0){
        result_num = string_get_range(tempTree, condition);
    }

    return result_num;
}

// 仅仅搜索一个值，返回值在buffer中的位置
Pointer IndexManager::SearchRetKey(string tableName, Condition condition)
{
    string indexName;
    bplus_tree *tempTree;
    long long data;
    Data key;

    for(auto it : treeList){
        if(it.second.tableName == tableName && it.second.attribute == condition.attribute){
            indexName = it.first;
            tempTree = activeTreeList[indexName];
            // cout << "name: " << indexName << endl;
        }
    }
    key = condition.value;

    if(condition.relation == EQUAL || condition.relation == LARGE_EQUAL || condition.relation == LESS_EQUAL){
        if(condition.value.type == 0){
            data = intTree.bplus_tree_get(tempTree, key.datai);
        }
        else if(condition.value.type < 0){
            data = floatTree.bplus_tree_get(tempTree, key.dataf);
        }
        else{
            data = stringTree.bplus_tree_get(tempTree, key.datas.content);
        }
    }
    else if(condition.relation == LARGE){
        if(condition.value.type == 0){
            data = intTree.bplus_tree_get_con(tempTree, key.datai, 1);
        }
        else if(condition.value.type < 0){
            data = floatTree.bplus_tree_get_con(tempTree, key.dataf, 1);
        }
        else{
            data = stringTree.bplus_tree_get_con(tempTree, key.datas.content, 1);
        }
    }
    else if(condition.relation == LESS){
        if(condition.value.type == 0){
            data = intTree.bplus_tree_get_con(tempTree, key.datai, -1);
        }
        else if(condition.value.type < 0){
            data = floatTree.bplus_tree_get_con(tempTree, key.dataf, -1);
        }
        else{
            data = stringTree.bplus_tree_get_con(tempTree, key.datas.content, -1);
        }
    }
    return fromData2Ptr(data);
}

// 从B+树中删除一个key
void IndexManager::deleteKey(string tableName, string Attr, Data key)
{
    bplus_tree *tempTree;
    string indexName;
    for(auto it : treeList){
        if(it.second.tableName == tableName && it.second.attribute == Attr){
            indexName = it.first;
            tempTree = activeTreeList[it.first];
        }
    }
    if(key.type == 0){
        intTree.bplus_tree_put(tempTree, key.datai, 0);
    }
    else if(key.type < 0){
        floatTree.bplus_tree_put(tempTree, key.dataf, 0);
    }
    else{
        stringTree.bplus_tree_put(tempTree, key.datas.content, 0);
    }
}

// 更新key值对应的ptr属性
void IndexManager::updateKey(string tableName, string Attr, Data key, Pointer _ptr)
{
    bplus_tree *tempTree;
    long long data;
    for(auto it : treeList){
        if(it.second.tableName == tableName && it.second.attribute == Attr){
            tempTree = activeTreeList[it.first];
        }
    }
    data = generateData(_ptr);
    if(key.type == 0){
        intTree.bplus_tree_put(tempTree, key.datai, 0);
        intTree.bplus_tree_put(tempTree, key.datai, data);
    }
    else if(key.type < 0){
        floatTree.bplus_tree_put(tempTree, key.dataf, 0);
        floatTree.bplus_tree_put(tempTree, key.dataf, data);
    }
    else{
        stringTree.bplus_tree_put(tempTree, key.datas.content, 0);
        stringTree.bplus_tree_put(tempTree, key.datas.content, data);
    }
}

// 将key值和对应的ptr插入到B+树中
void IndexManager::insertKey(string tableName, string Attr, Data key, Pointer _ptr)
{
    bplus_tree *tempTree;
    long long data;
    for(auto it : treeList){
        if(it.second.tableName == tableName && it.second.attribute == Attr){
            tempTree = activeTreeList[it.first];
        }
    }
    data = generateData(_ptr);
    if(key.type == 0){
        intTree.bplus_tree_put(tempTree, key.datai, data);
    }
    else if(key.type < 0){
        floatTree.bplus_tree_put(tempTree, key.dataf, data);
    }
    else{
        stringTree.bplus_tree_put(tempTree, key.datas.content, data);
    }
    
}

// 根据索引名向上层的函数返回一个索引结构
Index IndexManager::getIndex(string indexName)
{
    Index retIndex;
    for(auto it : treeList){
        if(it.first == indexName){
            retIndex.indexName = it.first;
            retIndex.tableName = it.second.tableName;
            retIndex.attribute = it.second.attribute;
        }
    }
    return retIndex;
}

// 按表名删除对应的索引
void IndexManager::dropTableIndex(string tableName)
{
    vector<string> dropList;
    for(auto tree : treeList){
        if(tree.second.tableName == tableName){
            dropList.push_back(tree.second.indexName);
        }
    }
    for(auto it : dropList){
        dropIndex(it);
    }
}

// 使用一个long long类型的变量存放BlockID和offset，分别放在高32位和低32位，从ptr类型的结构获取到对应的long long类型data
long long IndexManager::generateData(Pointer _ptr)
{
    long long result;
    result = _ptr.blockID;
    result = result << 32;
    result += _ptr.offset;
    return result;
}

// 从long long类型data获取到对应类型的结构ptr
Pointer IndexManager::fromData2Ptr(long long data)
{
    Pointer ptr;
    long long temp;
    ptr.blockID = data >> 32;
    temp = ptr.blockID;
    ptr.offset = data - (temp << 32);
    return ptr;
}

// 获取变量对应的长度
int IndexManager::getAttrSize(Attribute Attr)
{
    int result;
    if(Attr.type == 0){
        result = sizeof(int);
    }
    else if(Attr.type < 0){
        result = sizeof(float);
    }
    else if(Attr.type > 0){
        result = Attr.type;
    }

    return result;
}

int IndexManager::int_get_range(bplus_tree *tempTree, Condition condition)
{
    int val_num = 0;

    if(condition.relation == LESS){
        val_num = intTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datai, 0, 1, 1, TM);
    }
    else if(condition.relation == LESS_EQUAL){
        val_num = intTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datai, 0, 0, 1, TM);
    }
    else if(condition.relation == LARGE){
        val_num = intTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datai, 1, 0, 0, TM);
    }
    else if(condition.relation == LARGE_EQUAL){
        val_num = intTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datai, 0, 0, 0, TM);
    }
    else if(condition.relation == NOT_EQUAL){
        // cout << "neq!!!" << endl;
        val_num = intTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datai, 1, 1, 0, TM);
    }
    else if(condition.relation == EQUAL){
        val_num = intTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datai, 1, 1, 1, TM);
    }

    return val_num;
}

int IndexManager::float_get_range(bplus_tree *tempTree, Condition condition)
{
    int val_num = 0;

    if(condition.relation == LESS){
        val_num = floatTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.dataf, 0, 1, 1, TM);
    }
    else if(condition.relation == LESS_EQUAL){
        val_num = floatTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.dataf, 0, 0, 1, TM);
    }
    else if(condition.relation == LARGE){
        val_num = floatTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.dataf, 1, 0, 0, TM);
    }
    else if(condition.relation == LARGE_EQUAL){
        val_num = floatTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.dataf, 0, 0, 0, TM);
    }
    else if(condition.relation == NOT_EQUAL){
        val_num = floatTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.dataf, 1, 1, 0, TM);
    }
    else if(condition.relation == EQUAL){
        val_num = floatTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.dataf, 1, 1, 1, TM);
    }

    return val_num;
}

int IndexManager::string_get_range(bplus_tree *tempTree, Condition condition)
{
    int val_num = 0;

    if(condition.relation == LESS){
        val_num = stringTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datas.content, 0, 1, 1, TM);
    }
    else if(condition.relation == LESS_EQUAL){
        val_num = stringTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datas.content, 0, 0, 1, TM);
    }
    else if(condition.relation == LARGE){
        val_num = stringTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datas.content, 1, 0, 0, TM);
    }
    else if(condition.relation == LARGE_EQUAL){
        val_num = stringTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datas.content, 0, 0, 0, TM);
    }
    else if(condition.relation == NOT_EQUAL){
        val_num = stringTree.bplus_tree_get_range(tempTree, _ret_file, condition.value.datas.content, 1, 1, 0, TM);
    }
    else if(condition.relation == EQUAL){
        val_num = stringTree.bplus_tree_get_range(tempTree, _ret_file, IndexString(condition.value.datas.content), 1, 1, 1, TM);
    }

    return val_num;
}
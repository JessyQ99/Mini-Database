#include "api.h"

#include <iostream>

// 接受所要创建的表元数据
// 执行创建表的操作
// 返回受影响的行数（好像永远为0？）
int API::createTable(TableMeta tableMeta)
{
    CM.createTable(tableMeta);
    RM.createTable(tableMeta);
    Index index;
    index.tableName = tableMeta.tableName;
    for(auto key : tableMeta.primaryKeyVec) {
        index.attribute = key;
        // 后面加乱码，防止indexName与用户定义的重复
        index.indexName = "primaryIndex" + tableMeta.tableName + key + "dshafkg";
        IM.createIndex(index, CM.getType(index.tableName, index.attribute));
    }
    return 0;
}
// 接收所要删除的表名
// 删除表
// 返回被删的行数
int API::dropTable(string tableName)
{
    int count = RM.deLete(CM.getMeta(tableName), Where());
    RM.dropTable(tableName);
    IM.dropTableIndex(tableName);
    CM.dropTable(tableName);
    return count;
}
// 接收所要创建的索引名，索引所在的表名，要建立索引的属性名
// 创建索引
// 返回受影响的行数（mysql会返回受影响行数）
int API::createIndex(Index index)
{  
    int number = 0;
    if(CM.isAttrUnique(index.tableName, index.attribute)) { 
        IM.createIndex(index, CM.getType(index.tableName, index.attribute));
        number = RM.createIndex(CM.getMeta(index.tableName), index.attribute);
        CM.addIndex(index.tableName, index.attribute);
    }
    else {
        throw Index_Attribute_Not_Unique_Error(index.attribute);
    }
    return number;
}
// 接收要删除的索引名
// 删除索引
// 返回受影响的行数
int API::dropIndex(string indexName)
{
    Index index = IM.getIndex(indexName);
    if(!index.tableName.empty()) {
        IM.dropIndex(indexName);
        CM.removeIndex(index.tableName, index.attribute);
    }
    else {
        throw Index_Not_Exist_Error(indexName);
    }
    return 0;
}
// 接收表名、查询条件where和储存结果的Table对象
// 把表对应的元数据、符合条件的tuple写入result
// 返回tuple数量
int API::select(string tableName, Where where)
{
    if(!where.conditionVec.empty()) {
        if(!CM.isExistIndex(tableName, where.conditionVec[0].attribute)) {
            for(int i = 1; i < where.conditionVec.size(); i++) {
                if(CM.isExistIndex(tableName, where.conditionVec[i].attribute)) {
                    swap(where.conditionVec[0], where.conditionVec[i]);
                    break;
                }
            }
        }
    }
    return RM.select(CM.getMeta(tableName), where);
}
// 接收表名
// 删除表中所有tuple
// 返回被删除的tuple数量
int API::deleteTuple(string tableName)
{
    return RM.deLete(CM.getMeta(tableName), Where());
}
// 接收表名、删除条件
// 删除表中符合条件的tuple
// 返回被删除的tuple数量
int API::deleteWhere(string tableName, Where where)
{
    return RM.deLete(CM.getMeta(tableName), where);
}
// 接收插入数据的表名，所要插入的Tuple
// 向表中插入Tuple
// 返回插入的数量
int API::insert(string tableName, Tuple tuple)
{
    RM.insert(CM.getMeta(tableName), tuple);
    return 1;
}
#ifndef API_H
#define API_H

#include "DS.h"
#include "RecordManager.h"
#include "CatalogManager.h"
#include "IndexManager.h"

class API
{
public:
    API() : RM(IM, TM), IM(TM){};
    ~API() {};
    // 所要创建的表元数据
    // 执行创建表的操作
    // 返回受影响的行数（好像永远为0？）
    int createTable(TableMeta tableCata);
    // 接收所要删除的表名
    // 删除表
    // 返回被删的行数
    int dropTable(string tableName);
    // 接收所要创建的索引名，索引所在的表名，要建立索引的属性名
    // 创建索引
    // 返回受影响的行数（mysql会返回受影响行数）
    int createIndex(Index index);
    // 接收要删除的索引名
    // 删除索引
    // 返回受影响的行数
    int dropIndex(string indexName);
    // 接收表名、查询条件where和储存结果的Table对象
    // 把表对应的元数据、符合条件的tuple写入result
    // 返回tuple数量
    int select(string tableName, Where where);
    // 接收表名
    // 删除表
    // 返回被删除的tuple数量
    int deleteTuple(string tableName);
    // 接收表名、删除条件
    // 删除表中符合条件的tuple
    // 返回被删除的tuple数量
    int deleteWhere(string tableName, Where where);
    // 接收插入数据的表名，所要插入的Tuple
    // 向表中插入Tuple
    // 返回插入的数量
    int insert(string tableName, Tuple tuple);
    // 下面两个函数用于在Interpreter中解析where语句中的数据类型
    TableMeta getMeta(string tableName) {
        return CM.getMeta(tableName);
    }
    TYPE getType(string tableName, string attributeName) {
        return CM.getType(tableName, attributeName);
    }
private:
    BufferManager TM;
    CatalogManager CM;
    IndexManager IM;
    RecordManager RM;
};

#endif
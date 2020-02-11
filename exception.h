#ifndef EXCEPTION_H
#define EXCEPTION_H

#include <exception>
#include <string>

using namespace std;

// SQL语句错误
class query_formation_error : public std::exception {
    
};

// 建表时，表名/属性名非法错误
class name_illegal_error : public std::exception {
public:
    string name;
    name_illegal_error(string name) : name(name) {}
};

// 针对字符串类型，在插入/where语句中，参数长度大于类型长度
class string_overflow_error : public std::exception {
public:
    string name;
    string_overflow_error(string name) : name(name) {}    
};

// 用于execfile命令，执行文件不存在
class file_open_error : public std::exception {
public:
    string fileName;
    file_open_error(string fileName) : fileName(fileName) {
        
    }
};


class table_illegal_error : public std::exception {
public:
    string tableName;
    table_illegal_error(string tableName) : tableName(tableName) {}
};

// 索引属性非法异常
class attribute_illegal_error : public std::exception {

};

class index_illegal_error : public std::exception {

};

class index_already_exit_error :public std::exception {

};

class table_already_exit_error :public std::exception {
public:
    string tableName;
    table_already_exit_error(string tableName) : tableName(tableName) {}
};

class FileIO_error :public std::exception {

};

// 用于查询条件判断等，查询属性名未在表中异常
class Attribute_Not_Exist_Error : public std::exception {
public:
    string attriName;
    Attribute_Not_Exist_Error(string attriName) : attriName(attriName) {}
};

// 关系枚举类型错误
class Relation_Not_Exist_Error : public std::exception {

};

// 逻辑枚举类型错误
class Logic_Not_Exist_Error : public std::exception {

};

class Block_Empty_Error : public std::exception {

};

// 插入时，不符合unique约束错误
class Duplicate_Error : public std::exception {
public:
    string attribute;
    string value;
    Duplicate_Error(string attribute, string value) : attribute(attribute), value(value) {}
};

// 建索引的属性非unique异常
class Index_Attribute_Not_Unique_Error : public std::exception {
public:
    string attribute;
    Index_Attribute_Not_Unique_Error(string attri) : attribute(attri) {

    }
};

// 删除索引时，索引名不存在异常
class Index_Not_Exist_Error : public std::exception {
public:
    string indexName;
    Index_Not_Exist_Error(string indexName) : indexName(indexName) {

    }
};

#endif
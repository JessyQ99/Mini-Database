#ifndef DS_H
#define DS_H

#include <string>
#include <vector>
#include <stdexcept>
#include <set>

#include "exception.h"

using namespace std;

// 约定 type = 0 为整形
//     type < 0 为float
//     type > 0 为字符串，数值为字符串的长度
typedef int TYPE;

// 用于where语句中大小关系的判断
// 如 a > b
// that is, LARGE
typedef enum {
	LESS,
	LESS_EQUAL,
	LARGE,
	LARGE_EQUAL,
	EQUAL,
	NOT_EQUAL
} RELATION;

// 用于where语句中多个逻辑语句的联合
// 如 a > 1 and b < 1
// that is, AND
typedef enum {
	AND,
	OR
} LOGIC_OP;

// 可以用 strcpy(dest, content.c_str())转换为c字符串
// 然后在以maxSize大小写入文件
struct Varchar
{
	string content;
	int maxSize;

	Varchar(string s, int m) {
		if(s.length() > m)
			throw string_overflow_error(content);
		else {
			content = s;
			maxSize = m;
		}
	}

	bool operator==(Varchar v) {
		return content == v.content;
	}

	bool operator!=(Varchar v) {
		return content != v.content;
	}

	bool operator<(Varchar v) {
		return content < v.content;
	}

	bool operator<=(Varchar v) {
		return content <= v.content;
	}

	bool operator>(Varchar v) {
		return content > v.content;
	}

	bool operator>=(Varchar v) {
		return content >= v.content;
	}

	Varchar() = default;
};

// 数据的数据结构，用于接口间传递结果，主要用在Tuple类中
struct Data
{
	bool operator==(Data d) {
		if((d.type == type) || d.type * type > 0) {
			if(d.type == 0)
				return d.datai == datai;
			else if(d.type < 0)
				return d.dataf == dataf;
			else
				return d.datas == datas;
		}
		else
			return false;
	}
	bool operator!=(Data d) {
		if((d.type == type) || d.type * type > 0) {
			if(d.type == 0)
				return d.datai != datai;
			else if(d.type < 0)
				return d.dataf != dataf;
			else
				return d.datas != datas;
		}
		else
			return false;
	}
	bool operator<(Data d) {
		if((d.type == type) || d.type * type > 0) {
			if(d.type == 0)
				return d.datai > datai;
			else if(d.type < 0)
				return d.dataf > dataf;
			else
				return d.datas > datas;
		}
		else
			return false;
	}
	bool operator<=(Data d) {
		if((d.type == type) || d.type * type > 0) {
			if(d.type == 0)
				return d.datai >= datai;
			else if(d.type < 0)
				return d.dataf >= dataf;
			else
				return d.datas >= datas;
		}
		else
			return false;
	}
	bool operator>=(Data d) {
		if((d.type == type) || d.type * type > 0) {
			if(d.type == 0)
				return d.datai <= datai;
			else if(d.type < 0)
				return d.dataf <= dataf;
			else
				return d.datas <= datas;
		}
		else
			return false;
	}
	bool operator>(Data d) {
		if((d.type == type) || d.type * type > 0) {
			if(d.type == 0)
				return d.datai < datai;
			else if(d.type < 0)
				return d.dataf < dataf;
			else
				return d.datas < datas;
		}
		else
			return false;
	}
	int datai;
	float dataf;
	Varchar datas;
	TYPE type;
};
// 一行
typedef vector<Data> Tuple;

// 属性
struct Attribute
{
	// 属性名
	string name;
	// 属性类型
	// 约定 type = 0 为整形
	//     type < 0 为float
	//     type > 0 为字符串，数值为字符串的长度
	TYPE type;
	// 是否唯一
	bool isUnique;
	// 是否已经建立了索引
	bool isIndex;
	Attribute() : isUnique(false), isIndex(false){}
};

// 索引结构，索引名与对应的属性名
struct Index
{
	// 索引名
	string indexName;
	string tableName;
	string attribute;
};

// 表的元数据
struct TableMeta
{
	string tableName;
	std::vector<Attribute> attributeVec;
	vector<string> primaryKeyVec;
};

class Table
{
	TableMeta tableCata;
	vector<Tuple> tupleVec;
};

// 子关系结构，用于where语句
// where name = "Yangjianwei"
// => Condition("name", "Yangjianwei", EQUAL);
// 与Where结构共同使用
struct Condition
{
public:
	// 对应的值
	Condition(string attributeName, Data value, RELATION relation) : attribute(attributeName), relation(relation), value(value) {
	}
	// 属性名
	string attribute;
	// 大小关系
	RELATION relation;
	// 比较的值
	Data value;
};

struct Where
{
	vector<Condition> conditionVec;
	vector<LOGIC_OP> opVec;
};

struct Pointer
{
	// 所在的块号
	int blockID;
	// 所在块的偏移量
	int offset;
	Pointer(int blockID, int offset) : blockID(blockID), offset(offset) {}
	Pointer() = default;
	bool operator==(const Pointer ptr) {
		return ptr.blockID == blockID && ptr.offset == offset;
	}
};

#endif
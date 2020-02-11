# Mini Database

## 一、任务目标

设计并实现一个精简型单用户SQL引擎(DBMS)MiniSQL，允许用户通过字符界面输入SQL语句实现表的建立/删除；索引的建立/删除以及表记录的插入/删除/查找。

通过对MiniSQL的设计与实现，提高系统编程能力，加深对数据库系统原理的理解。

## 二、系统需求

### 2.1    需求概述

#### 2.1.1 数据类型

支持三种基本数据类型：int，char(n)，float，其中char(n)满足 1 <= n <= 255 。

#### 2.1.2 表定义

一个表最多可以定义32个属性，各属性可以指定是否为unique；支持单属性的主键定义。

#### 2.1.3 索引的建立和删除

对于表的主属性自动建立B+树索引，对于声明为unique的属性可以通过SQL语句由用户指定建立/删除B+树索引。

#### 2.1.4 查找记录

可以通过指定用and连接的多个条件进行查询，支持等值查询和区间查询。

#### 2.1.5 插入和删除记录

支持每次一条记录的插入操作；支持每次一条或多条记录的删除操作。

### 2.2    语法说明 

MiniSQL支持标准的SQL语句格式，每一条SQL语句以分号结尾，一条SQL语句可写在一行或多行。所有的关键字都需为小写。

#### 2.2.1 创建表语句

该语句的语法如下：

```sql
create table 表名 (

     列名 类型 ,

     列名 类型 ,

     列名 类型 ,

     primary key ( 列名 )

);
```

该语句执行成功，则输出执行成功信息；若失败，输出用户失败的原因。

示例语句：

```sql
create table student (

        sno char(8),

        sname char(16) unique,

        sage int,

        sgender char (1),

        primary key ( sno )

);
```

#### 2.2.2 删除表语句

该语句的语法如下：

```sql
drop table 表名;
```

若该语句执行成功，则输出执行成功信息；若失败，输出用户失败的原因。

示例语句：

```sql
drop table student;
```

#### 2.2.3 创建索引语句

该语句的语法如下：

```sql
create index 索引名 on 表名 ( 列名 );
```

若该语句执行成功，则输出执行成功信息；若失败，输出用户失败的原因。

示例语句：

```sql
create index stunameidx on student ( sname );
```

#### 2.2.4 删除索引语句

该语句的语法如下：

```sql
drop index 索引名 ;
```

若该语句执行成功，则输出执行成功信息；若失败，输出用户失败的原因。

示例语句：

```sql
drop index stunameidx;
```

#### 2.2.5 选择语句

该语句的语法如下：

```sql
select * from 表名 ;
```

或：

```sql
select * from 表名 where 条件 ;
```

其中“条件”具有以下格式：列 op 值 and 列 op 值 … and 列 op 值。

op是算术比较符：=    <>      <        >        <=      >=

若该语句执行成功且查询结果不为空，则按行输出查询结果，第一行为属性名，其余每一行表示一条记录；若查询结果为空，则输出查询结果为空；若失败，输出用户失败的原因。

示例语句：

```sql
select * from student;

select * from student where sno = ‘88888888’;

select * from student where sage > 20 and sgender = ‘F’;
```

#### 2.2.6 插入记录语句

该语句的语法如下：

```sql
insert into 表名 values ( 值1 , 值2 , … , 值n );
```

若该语句执行成功，则输出执行成功信息；若失败，输出用户失败的原因。

示例语句：

```sql
insert into student values (‘12345678’,’wy’,22,’M’);
```

#### 2.2.7 删除记录语句

该语句的语法如下：

```sql
delete from 表名 ;
```

或：

```sql
delete from 表名 where 条件 ;
```

若该语句执行成功，则输出执行成功信息，其中包括删除的记录数；若失败，输出用户失败的原因。

示例语句：

```sql
delete from student;

delete from student where sno = ‘88888888’;
```

#### 2.2.8 退出MiniSQL系统语句

该语句的语法如下：

```sql
quit;
```

#### 2.2.9 执行SQL脚本文件语句

该语句的语法如下：

```sql
execfile 文件名 ;
```

SQL脚本文件中可以包含任意多条上述8种SQL语句，MiniSQL系统读入该文件，然后按序依次逐条执行脚本中的SQL语句。

## 三、实验环境

### 3.1 编译环境

因为 B+ 树使用了POISX标准函数，所以推荐在UNIX系统下编译

在windows系统下，需要使用MinGW编译

在代码目录下，有Makefile，可以直接使用make命令编译

### 3.2 运行环境

支持全平台，在相应的系统下编译即可

注意，可执行文件当前目录下需要创建index、record、catalog三个文件夹

## 四、模块设计

### 4.0 数据结构设计

数据结构会在接下来的三个模块中使用到，在此列出并解释，在下面模块中不再赘述。

```cpp
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
```

###4.1 Interpreter 模块

#### 4.1.1 模块描述

##### 4.1.1.1 整体描述

Interpreter模块直接与用户交互，主要实现以下功能：

1. 程序流程控制，即启动并初始化接收命令、处理命令、显示命令结果循环至退出”流程。
2. 接收并解释用户输入的命令，生成命令的内部数据结构表示，同时检查命令的语法正确性和语义正确性，对正确的命令调用API层提供的函数执行并显示执行结果，对不正确的命令显示错误信息。

##### 4.1.1.2 功能描述

- 提供minisql运行主循环
- 接收SQL语句
- 解析SQL语句，提取关键字信息和参数信息
- 调用API中函数

#### 4.1.2 接口设计

```cpp
class Interpreter
{
public:
	Interpreter();
	~Interpreter();
	// 进入 minisql 程序主循环
	void mainLoop();
private:
	...
};
```

#### 4.1.3 调用关系

![a](./解析.jpg)

### 4.2 API 模块

####4.2.1 模块描述

##### 4.2.1.1 整体描述

API模块是整个系统的核心，其主要功能为提供执行SQL语句的接口，供Interpreter层调用。该接口以Interpreter层解释生成的命令内部表示为输入，根据Catalog Manager提供的信息确定执行规则，并调用Record Manager、Index Manager和Catalog Manager提供的相应接口进行执行，最后返回执行结果给Interpreter模块。

##### 4.2.1.2 功能描述

- 初步判断参数是否异常
- 管理模块使用的Buffer类
- 统一管理各个模块
- 调用各个模块，完成query

#### 4.2.2 接口设计

```cpp
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
    ...
};
```

#### 4.2.4 调用关系

![a](./API模块接口.jpg)

### 4.3 RecordManager 模块

#### 4.3.1 模块描述

##### 4.3.1.1 整体描述

Record Manager负责管理记录表中数据的数据文件。主要功能为实现数据文件的创建与删除（由表的定义与删除引起）、记录的插入、删除与查找操作，并对外提供相应的接口。其中记录的查找操作要求能够支持不带条件的查找和带一个条件的查找（包括等值查找、不等值查找和区间查找）。

数据文件由一个或多个数据块组成，块大小应与缓冲区块大小相同。一个块中包含一条至多条记录，为简单起见，只要求支持定长记录的存储，且不要求支持记录的跨块存储。

##### 4.3.1.2 功能描述

- 支持多条件单索引的查询
- 支持多条件的删除
- 创建表
- 删除表
- 支持约束的插入
- 查询传参使用工作缓冲区完成，理论可查询与磁盘等大的数据

#### 4.3.2 接口设计

```cpp
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
	...
};
```

#### 4.3.3 调用关系

![a](./Record.jpg)

## 五、模块实现

### 5.1 Interpreter 模块

#### 5.1.1 实现原理

Interpreter通过语法树逐词来进行解析，通过多个if-else逻辑进行判断sql是否合法并提取相应的关键字。具体实现上分为以下几个步骤：

###### （1）预处理

由于Interpreter通过空格进行分词而SQL语句对特殊符号不要求添加空格，所以需要对SQL语句进行第一遍扫描，将特殊符号前后没有空格的加上空格。另外需要注意的是，由于通过;判断SQL是否结束，还需要将末尾的空格回车等字符删去

```cpp
void Interpreter::pretreatment(string &cmd)
{
    bool isString = false;
    // 在所有的特殊符号的前后增加一个空格以便于提取关键字
    for(int pos=0;pos<cmd.length();pos++){
        if(cmd[pos] == '\'' || cmd[pos] == '\"')
            isString = !isString;
        if(isString)
            continue;
        if(cmd[pos] == '*' || cmd[pos] == '=' || cmd[pos] == ','
            || cmd[pos] == '(' || cmd[pos] == ')' || cmd[pos] == '<' || cmd[pos] == '>' || cmd[pos] == '-' ){
            if(cmd[pos-1] != ' ')
                cmd.insert(pos++," ");
            if(cmd[pos+1]!=' ')
                cmd.insert(++pos," ");
        }
        else if(cmd[pos] == ';')
            if(cmd[pos-1] != ' ')
                cmd.insert(pos++," ");
    }
}
```

###### (2) 解析query

query的解析是基于语法树的分析，不断地进行if-else判断，直到执行到对应的API为止，这里需要注意的是针对execfile的执行和where语句的解析，execfile本质是循环调用parse_query，也就是一个递归。针对where语句，我们需要分属性的类型构造合适的对象解析数据。此处代码较长，不再贴出，仅给出parseWhere的实现逻辑。

```cpp
// 解析where语句
Where Interpreter::parseWhere(stringstream &cmdline, string tableName)
{
    Where where;
    string keyword = "strat";

    // where sage > 20 and sgender = ‘F’;
    string attribute;
    string relationStr;
    Data value;
    // should get attribute type here
    TYPE type = 0;
    RELATION relation;
    LOGIC_OP logic;

    while (keyword != ";")
    {
        cmdline >> attribute;
        cmdline >> relationStr;
        cmdline >> keyword;
        if(relationMap.find(keyword) != relationMap.end()) {
            relationStr += keyword;
            cmdline >> keyword;
        }
        if(relationMap.find(relationStr) != relationMap.end())
            relation = relationMap[relationStr];
        else
            throw query_formation_error(); 
        type = api.getType(tableName, attribute);
        
        if(type == 0) {
            value.type = 0;
            value.datai = parseInt(keyword);
        }
        else if(type < 0) {
            value.type = -1;
            value.dataf = parseFloat(keyword);
        }
        else if(type > 0) {
            value.type = type;
            value.datas.content = parseString(keyword);
            value.datas.maxSize = type;
        } 

        Condition condition(attribute, value, relation);
        where.conditionVec.push_back(condition);
        cmdline >> keyword;
        if(logicMap.find(keyword) != logicMap.end()) {
            where.opVec.push_back(logicMap[keyword]);
        }
        else if(keyword == ";")
            ;
        else 
            throw query_formation_error();
    }

    return where;
}
```

#### 5.1.2 单元测试

该模块与API模块紧密依赖，故将Interpreter的单元测试与API的单元测试合并，在下一节中给出测试方法与测试结果

### 5.2 API 模块

#### 5.2.1 实现原理

API模块负责统一调度Catalog模块、Index模块与RecordManager模块，并将内存资源，也就是Buffer，分发给三个模块。

值得注意的是，API模块对条件查询进行了优化，将条件中带有索引的属性放在了首位。

#### 5.2.2 单元测试

###### （1）测试方法

通过Interpreter模块解析query传递到API模块，输出API模块到输出信息。

###### （2）测试结果

欢迎界面：

![a](./unitTest/welcome.png)

创建表：

![a](./unitTest/createTable.png)

无主键建表：

![a](./unitTest/createTableWithoutPrimary.png)

插入：

![a](./unitTest/insert.png)

查询：

![a](./unitTest/select.png)

删除：

![a](./unitTest/delete.png)

创建索引:

![a](./unitTest/createIndex.png)

删除索引：

![a](./unitTest/dropIndex.png)

删除表：

![a](./unitTest/dropTable.png)

错误的SQL语句

![a](./unitTest/wrongQuery.png)

退出sql：

![a](./unitTest/quit.png)

如上所示，测试结果均与理想输出一致，单元测试通过。

### 5.3 RecordManager 模块

#### 5.3.1 实现原理

##### 5.3.1.1 CreateTable

createTable 创建记录文件并把表的文件信息写入Buffer的文件管理

```cpp
int RecordManager::createTable(TableMeta meta)
{
    string filePath = "./record/" + meta.tableName;
    FILE *fp = fopen(filePath.c_str(), "w");
    fclose(fp);
    BM.setFileInfoInitialized(filePath);

    return 0;
}
```

##### 5.3.1.2 Insert

首先判断各个属性上的约束是否成立，也就是对每一个Unique的属性，在记录中使用select方法检测是否唯一。

之后，根据BufferManage模块中记录的文件信息，直接把记录插入到文件的末尾

最后，检测存在索引的属性，把键值插入到B+树中

```cpp
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
```

##### 5.3.1.3 Select

Select由多个功能函数完成，一个是线性搜索的无条件查询，一个是首条件单条件查询，一个是求与的单条件查询，另一个是求或的单条件查询。以上函数组合完成由无条件查询到多条件查询的功能，为了实现极小内存假设，在极小内存内完成查询，我们没有使用内存传递参数，而是将查询到的记录指针写入到工作区，有selectPrint完成打印。

实现逻辑是这样的，首先判断条件是否为空，如果为空则进行线性搜索，若不为空，则执行首条件单条件查询，之后循环检测条件是否为空与对应的逻辑操作码，执行对应的SelectAnd / SelectOr，直至条件全部使用完全为止。

求与的单条件查询从上次查询的工作区开始扫描，将符合条件的记录从工作区首地址开始写入，如此做到了O(1)的求与。求或的单条件查询需要对每条符合数据进行一次存在性判断，也就是扫描一遍工作区文件，将符合条件且不在工作区的记录指针添加到工作区末尾。

对索引的支持是在首条件单条件查询，首先判断属性上是否存在索引，若存在索引，则通过索引搜索，若不存在，则线性搜索。

##### 5.3.1.4 Delete

Delete对每一条记录进行删除条件判断，若记录需要被删除，则使用最后一条记录覆盖他，并把最后一条记录删除，若删除的记录是最后一条记录，则直接删除。

这样设计使得插入和删除的时间复杂度都达到了最小。

```cpp
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
```

##### 5.3.1.5 dropTable

首先调用无条件的delete，将记录全部删除，之后将表的文件删除，再把表的文件头信息从缓冲区中删除。

```cpp
int RecordManager::dropTable(string tableName)
{
    // 根据usedbit计算有多少个record
    string filePath = "./record/" + tableName;
    BM.removeFileInfo(filePath);
    BM.deleteFrames(filePath);
    remove(filePath.c_str());

    return 0;
}
```

##### 5.3.1.6 createIndex

把表中所有记录的key插入到B+树中

```cpp
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
```

#### 5.3.2 单元测试

由于RecordManager模块与其他所有模块都具有高度依赖性，故单元测试约等于整体测试，以下是测试情况：

首轮查询主要以执行各种SQL语句为主，并验证建立索引前后单点查询、区间查询时间对比。对于涉及到多条记录的查询，将利用MySQL查询结果进行对比验证。

##### Testcase 1

##### select * from student2 where id=1080100245; --考察int类型上的等值条件查询 

![1](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image001.png)

##### Testcase 2

##### select * from student2 where score=98.5; --考察float类型上的等值条件查询，观察数量 

##### ![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image003.png)![2,2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image004.png)

 

##### Testcase 3 

##### select * from student2 where name='name245'; 线性查询时间t1

![3](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image005.png)

Testcase 11对比增加索引后利用索引查询时间t2

![11](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image006.png)

t1<t2，索引查询效率高。

 

##### Testcase 4

##### select * from student2 where id<>1080109998; --考察int类型上的不等条件查询，观察数量

![4.2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image008.png)

Testcase 5 

 select \* from student2 where score<>98.5; --考察float类型上的不等条件查询，观察数量

![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image009.png)![5.2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image011.png)

Testcase 6  

select \* from student2 where name<>'name9998’; --考察char类型上的不等条件查询，观察数量

![6.2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image013.png)    

Testcase 7

select \* from student2 where score>80 and score<85; --考察多条件and查询，观察数量，对比MySQL进行验证。

![7.2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image015.png)

![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image016.png)

 Testcase 8 

select \* from student2 where score>95 and id<=1080100100; --考察多条件and查询，观察数量 

![8](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image017.png)

​                               

对比Mysql验证正确性：

![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image018.png)

​                    

 

Testcase 9 

insert into student2 values(1080100245,'name245',100); --报primary key约束冲突（或报unique约束冲突）

![9](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image019.png)

Testcase 10 

create index stuidx on student2 ( score ); --unique key才能建立索引

create index stuidx on student2 ( name ); --在name这个unique属性上创建index

 

![10](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image020.png)

![10.2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image021.png)



Testcase 12

insert into student2 values(1080197996,’name97996’,100); --考察在建立b+树后再插入数据，b+树有没有做好insert

![12](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image022.png)

Testcase 13

select \* from student2 where name='name97996’; --观察索引查询时间t3

![13](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image023.png)

Testcase18

对比观察删除索引后查询时间t4

![18](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image024.png)

此处t3<t4,索引查询效率较高。

Testcase14 

delete from student2 where name='name97996’; --考察delete，同时考察b+树的delete

![14](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image025.png)

Testcase15

select \* from student2 where name='name97996’;

![15](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image026.png)

Testcase16 

insert into student2 values(1080197996,’name97996’,100);

![16](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image027.png)

 

Testcase 17

 select\*from student2 where name<'name10';区间查询，观察执行时间t5,有索引查询

![IMG_256](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image028.jpg)

 

Testcase 20

 select\*from student2 where name<'name10';区间查询，观察执行时间t6 无索引查询

![IMG_256](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image029.jpg)

Testcase 18

drop index stuidx; --考察drop index

![17](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image030.png)

 

Testcase 19

select \* from student2 where name='name97996’; --需观察此处的执行时间t4

![18](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image024.png)

 Testcase 21

delete from student2 where id=1080100245; --考察delete

![20](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image031.png)

 Testcase 22

select \* from student2 where id=1080100245;

![21](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image032.png)

 Testcase 23

 delete from student2 where score=98.5; --考察delete

![22](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image033.png)

![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image034.png)

 Testcase 24

 select \* from student2 where score=98.5;

表明已删除干净

![23](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image035.png)

 Testcase 25

delete from student2; --考察delete操作

![24](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image036.png)

对比Mysql

![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image037.png)

 Testcase 26

select \* from student2;

表明已删除干净

![25](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image038.png)

 Testcase 27 

drop table student2; --考察drop table

![26](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image039.png)

Testcase 28  

select \* from student2;

![27](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image040.png)

 

Testcase 29

quit

![28](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image041.png)

Testcase 30

测试execfile

![fileNotExist](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image042.png)

 Testcase 31

测试Welcome

![welcome](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image043.png)

二轮查询

此轮测试主要是测试文件写入是否正确，测试方法是建表、插入记录然后查询记录，退出程序后打开文件重新进行查询，对比两次查询结果看是否一致。

 

首先重新执行文件，插入记录。

![第二轮建表](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image044.png)

进行查询操作

![img](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image045.png)

![二轮建表查询结果](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image047.png)

![二轮重新查询2](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image049.png)

执行quit退出程序重新查询：



![二轮退出重新查询1](file:////Users/qsy/Library/Group%20Containers/UBF8T346G9.Office/TemporaryItems/msohtmlclip/clip_image051.png)

## 六、遇到的问题与解决方法

### 6.1 插入速度慢

最开始的插入效率较低，原因是为了提高delete的效率，在删除delete的中间记录时，选择把当前块下最后一条记录填补到删除的位置，这样虽然delete不存在跨块删除，但insert需要遍历所有的块来寻找空缺位置。

所以解决方法是稍微牺牲delete的效率，选择把最后一块的最后一条记录填充被删除的记录，在insert时使用Buffer中记录的值来记录长度，直接定位到尾部，在尾部插入。

这样，插入和删除的时间复杂度就都成为了O(1)

### 6.2 参数依赖于内存

在最开始的实现中，我们使用数组来将查询到的信息在模块间传递，但这样会导致查询的大小受限于机器内存的大小，也就是说，如果查询长度大于内存，查询就会失败，数据库应该有极小内存假设，所以这样的设计是不合理的。

于是我们采用指针文件传递参数，使用工作缓冲区装载指针文件，也就是说使用我们实现的虚存传递查询结果，这样，我们的查询长度就拓展到了与磁盘大小一致。

### 6.3 多条件查询的索引实现

多条件查询在最初使用的方法是完全线性搜索法，也就是对每一条记录都进行多条件判断，将符合条件的记录指针写入工作区。

但这样无法实现索引加速，因为线性搜索是多条件同时判断的，无法使用索引加速。所以优化是使用组合查询，也就是说分级，一个条件一个条件的进行筛选，在第二至第n级求与或求或从而实现使用任意多个索引查询的效果

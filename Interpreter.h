#ifndef INTERPRETER_H
#define INTERPRETER_H

// 用于解析命令并调用各个模块
// IO也在这个模块内

#include "DS.h"
#include "api.h"
#include <map>

// 检测名字是否合法(不包括除字母、数字、下划线以外的特殊字符，首字符不能为数字)
// 名字最长不超过20个字符
bool isLegal(const string name);

class Interpreter
{
public:
	Interpreter();
	~Interpreter() {

	};
	// main loop;
	void mainLoop();
private:
	// 预处理，将特殊符号前加空格
	void pretreatment(string &cmd);
	// 解析命令并调用API执行命令
	void parseQuery(const string &cmd);
	// 解析where语句
	Where parseWhere(stringstream &cmdline, string tableName);
	string parseString(string keyword);
	float parseFloat(string keyword);
	int parseInt(string keyword);
	map<string, TYPE> typeMap;
	map<string, RELATION> relationMap;
	map<string, LOGIC_OP> logicMap;
private:
	API api;
};

#endif
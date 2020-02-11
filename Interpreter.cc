#include "Interpreter.h"

#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <ctime>
#include "exception.h"

Interpreter::Interpreter()
{
    cout << "Welcome to the miniSQL monitor.  Commands end with ;." << endl;
    cout << "Your miniSQL connection id is 0" << endl;
    cout << "Server version: 0.0.1" << endl;
    cout << "Copyright (c) 2019, 2019,  Yang Jianwei, Guo Zejun, Qian Siyi. All rights reserved." << endl;
    cout << "We are a group of Database course and/or our" << endl;
    cout << "teacher is Zhou Bo. Other names who help us haven't" << endl;
    cout << "listed." << endl;
    cout << "Type 'quit' to exit" << endl;

    typeMap["int"] = 0;
    typeMap["float"] = -1;
    typeMap["char"] = 1;

    relationMap["="] = EQUAL;
    relationMap[">"] = LARGE;
    relationMap["<"] = LESS;
    relationMap[">="] = LARGE_EQUAL;
    relationMap["<="] = LESS_EQUAL;
    relationMap["<>"] = NOT_EQUAL;

    logicMap["and"] = AND;
    logicMap["or"] = OR;
}

// main loop
void Interpreter::mainLoop()
{
    string cmd = "over";

    while(1) {        
        cout << "minisql> ";
        getline(cin, cmd);
        // 去除末尾空格，防止干扰;检测
        cmd.erase(cmd.find_last_not_of(" ") + 1);
        cmd.erase(cmd.find_last_not_of("\n") + 1);
        cmd.erase(cmd.find_last_not_of("\r") + 1);
        if(cmd == "quit" || cmd == "quit;") {
            cout << "Bye" << endl;
            break;
        }
        while((cmd.empty() || (!cmd.empty() && cmd.back() != ';'))) {
            cout << "      -> ";
            string temp;
            getline(cin, temp);
            cmd += " " + temp;
            // 去除末尾空格，防止干扰;检测
            cmd.erase(cmd.find_last_not_of(" ") + 1);
            cmd.erase(cmd.find_last_not_of("\n") + 1);
            cmd.erase(cmd.find_last_not_of("\r") + 1);
        }
        // 预处理，增加空格
        pretreatment(cmd);
        // 解析query并执行
        parseQuery(cmd);
    }
}

void Interpreter::parseQuery(const string &cmd)
{
    stringstream cmdline(cmd);
    string keyword;
    cmdline >> keyword;

    try {
        // 第一个关键字为create
        if(keyword == "create") {
            cmdline >> keyword;
            if(keyword == "table") {
                TableMeta tableCata;
                bool isPrimaryExist = false;
                cmdline >> keyword;

                // 第三个关键字为表名
                if(isLegal(keyword))
                    tableCata.tableName = keyword;
                cmdline >> keyword;
                if(keyword != "(")
                    throw query_formation_error();

                
                string end;
                do
                {
                    string attributeName;
                    string typeStr;
                    Attribute attribute;

                    cmdline >> attributeName;
                    cmdline >> typeStr;

                    // 解析属性名和属性类型
                    if(attributeName != "primary") {
                        if(isLegal(attributeName) && typeMap.find(typeStr) != typeMap.end()) {
                            attribute.name = attributeName;
                            attribute.type = typeMap[typeStr];
                        }
                        else 
                            throw query_formation_error(); 
                        
                        if(typeStr == "char") {
                            cmdline >> end;
                            if(end != "(")
                                throw query_formation_error(); 
                            cmdline >> end;
                            for (const auto i : end) {
                                if(!isdigit(i))
                                    throw query_formation_error(); 
                            }
                            attribute.type = stoi(end);
                            cmdline >> end;
                            if(end != ")")
                                throw query_formation_error(); 
                        }
                        cmdline >> end;
                        if(end == "unique") {
                            attribute.isUnique = true;   
                            cmdline >> end;
                        }
                        attribute.isIndex = false;
                        tableCata.attributeVec.push_back(attribute);
                    }
                    else {
                        if(typeStr == "key") {
                            // primary key (id)
                            cmdline >> typeStr;
                            if(typeStr != "(")
                                throw query_formation_error();
                            cmdline >> attributeName;

                            int count = 0;
                            // 多个primary key的情况
                            while(attributeName != ")" && count <= tableCata.attributeVec.size()) {
                                bool isExist = false;
                                for(auto &i : tableCata.attributeVec) {
                                    if(attributeName == i.name) {
                                        tableCata.primaryKeyVec.push_back(attributeName);
                                        isExist = true;
                                        i.isUnique = true;
                                        i.isIndex = true;
                                        isPrimaryExist = true;
                                    }
                                }
                                if(!isExist) {
                                    throw query_formation_error();
                                }
                                count++;
                                cmdline >> attributeName;
                            }
                            
                            if(count > tableCata.attributeVec.size())
                                throw query_formation_error();

                            cmdline >> end;
                            if(end != ")")
                                throw query_formation_error();
                        }
                        else
                            throw query_formation_error();
                    }
                } while (end == ",");

                if(end == ")") {
                    cmdline >> end;
                    if(end != ";")
                        throw query_formation_error();
                }
                else
                    throw query_formation_error();
                
                if(!isPrimaryExist)
                    throw query_formation_error();

                auto startTime = clock();
                int count = api.createTable(tableCata);
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
                cout << endl;
            }
            // create index stunameidx on student ( sname );
            else if(keyword == "index") {
                cmdline >> keyword;
                string indexName;
                string attributeName;
                string tableName;
                if(isLegal(keyword)) {
                    indexName = keyword;
                    cmdline >> keyword;
                    if(keyword != "on")
                        throw query_formation_error();
                    cmdline >> keyword;
                    if(isLegal(keyword)) {
                        tableName = keyword;
                        cmdline >> keyword;
                        if(keyword != "(")
                            throw query_formation_error();
                        cmdline >> keyword;
                        attributeName = keyword;
                        cmdline >> keyword;
                        if(keyword != ")")
                            throw query_formation_error();
                        cmdline >> keyword;
                        if(keyword != ";")
                            throw query_formation_error();                           
                    }
                    else
                        throw query_formation_error();

                    Index index;
                    index.tableName = tableName;
                    index.indexName = indexName;
                    index.attribute = attributeName;

                    auto startTime = clock();
                    int count = api.createIndex(index);
                    auto endTime = clock();
                    double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                    cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
                    cout << "Records: 0  Duplicates: 0  Warnings: 0" << endl;
                    cout << endl;
                }
                else
                    throw query_formation_error();
                
            }
            else 
                throw query_formation_error();
        }
        else if(keyword == "drop") {
            cmdline >> keyword;

            // drop table student;
            if(keyword == "table") {
                string tableName;
                cmdline >> tableName;
                cmdline >> keyword;
                if(keyword != ";")
                    throw query_formation_error();

                auto startTime = clock();
                int count = api.dropTable(tableName);
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
                cout << endl;
            }
            // drop index stunameidx;
            else if(keyword == "index") {
                string indexName;
                cmdline >> indexName;
                cmdline >> keyword;
                if(keyword != ";")
                    throw query_formation_error();

                auto startTime = clock();
                int count = api.dropIndex(indexName);
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
                cout << "Records: 0  Duplicates: 0  Warnings: 0" << endl;
                cout << endl;
            }
            else 
                throw query_formation_error();
            

        }
        // select * from student;
        // select * from student where sno = ‘88888888’;
        // select * from student where sage > 20 and sgender = ‘F’;
        else if(keyword == "select") {
            string tableName;
            cmdline >> keyword;
            if(keyword != "*")
                throw query_formation_error();
            cmdline >> keyword;
            if(keyword != "from")
                throw query_formation_error();
            cmdline >> tableName;
            cmdline >> keyword;
            if(keyword == "where") {
                Where where = parseWhere(cmdline, tableName);
                auto startTime = clock();
                int count = api.select(tableName, where);
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << count << " row in set (" << fixed << setprecision(2) << runTime << " sec)" << endl;
                cout << endl;
            }
            else if (keyword == ";"){
                Where where;
                Table result;
                auto startTime = clock();
                int count = api.select(tableName, Where());
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << count << " row in set (" << fixed << setprecision(2) << runTime << " sec)" << endl;
                cout << endl;
            }
            else 
                throw query_formation_error();
        }
        // delete from student;
        // delete from student where sno = ‘88888888’;
        else if(keyword == "delete") {
            string tableName;
            cmdline >> keyword;
            if(keyword != "from")
                throw query_formation_error();
            cmdline >> tableName;
            cmdline >> keyword;
            if(keyword == "where") {
                Where where = parseWhere(cmdline, tableName);
                auto startTime = clock();
                int count = api.deleteWhere(tableName, where);
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
                cout << endl;
            }
            else if (keyword == ";"){
                auto startTime = clock();
                int count = api.deleteTuple(tableName);
                auto endTime = clock();
                double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
                cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
                cout << endl;
            }
            else 
                throw query_formation_error();
        }
        else if(keyword == "execfile") {
            string filePath;

            cmdline >> filePath;
            cmdline >> keyword;
            if(keyword != ";")
                throw query_formation_error();
            
            ifstream in(filePath);
            if(!in)
                throw file_open_error(filePath);
            
            
            while(!in.eof()) {   
                string cmd;
                getline(in, cmd);
                if(cmd == "quit" || cmd == "quit;")
                    break;
                // 去除末尾空格，防止干扰;检测
                cmd.erase(cmd.find_last_not_of(" ") + 1);
                cmd.erase(cmd.find_last_not_of("\n") + 1);
                cmd.erase(cmd.find_last_not_of("\r") + 1);
                while(!in.eof() && (cmd.empty() || (!cmd.empty() && cmd.back() != ';'))) {
                    cout << cmd.back() << endl;
                    string temp;
                    getline(in, temp);
                    cmd += " " + temp;
                    // 去除末尾空格，防止干扰;检测
                    cmd.erase(cmd.find_last_not_of(" ") + 1);
                    cmd.erase(cmd.find_last_not_of("\n") + 1);
                    cmd.erase(cmd.find_last_not_of("\r") + 1);
                }
                transform(cmd.begin(), cmd.end(), cmd.begin(), [](char c){ return c == '\n' ? ' ' : c; });
                // 预处理，增加空格
                pretreatment(cmd);
                // 解析query并执行
                parseQuery(cmd);
            }        
        }
        // insert into student values (‘12345678’,’wy’,22,’M’);
        else if(keyword == "insert") {
            cmdline >> keyword;
            if(keyword != "into")
                throw query_formation_error();
            string tableName;
            cmdline >> tableName;
            cmdline >> keyword;
            if(keyword != "values")
                throw query_formation_error();
            cmdline >> keyword;
            if(keyword != "(")
                throw query_formation_error();
            // TableCata meta = CatalogManager.getCata(tableName);
            Tuple tuple;
            TableMeta meta = api.getMeta(tableName);
            int i = 0;
            do
            {
                cmdline >> keyword;
                Data data;
                data.type = meta.attributeVec[i].type;
                if(data.type == 0)
                    data.datai = parseInt(keyword);
                else if(data.type < 0)
                    data.dataf = parseFloat(keyword);
                else {
                    try {
                        data.datas = Varchar(parseString(keyword), data.type);
                    } catch (string_overflow_error e) {
                        throw string_overflow_error(meta.attributeVec[i].name);
                    }
                }
                i++;
                tuple.push_back(data);
                cmdline >> keyword;
                if(keyword != "," && keyword != ")")
                    throw query_formation_error();
            } while (keyword != ")" && keyword != ";");
           
            auto startTime = clock();
            int count = api.insert(tableName, tuple);
            auto endTime = clock();
            double runTime = (double)(endTime - startTime) / CLOCKS_PER_SEC;
            cout << "Query OK, " << count << " rows affected (" << fixed << setprecision(2) << runTime <<" sec)" << endl;
            cout << endl;
        }
        else if(keyword == "quit" || keyword == "") {
            return;
        }
        else if(keyword == "-")
            ;
        else {
            cerr << "ERROR 1064 (42000): You have an error in your SQL syntax" << endl;
        }
    } catch(query_formation_error) {
        cerr << "ERROR 1064 (42000): You have an error in your SQL syntax;" << endl;
    } catch(table_illegal_error e) {
        cerr << "ERROR 1146 (42S02): Table '" << e.tableName << "' doesn't exist" << endl;
    } catch(Duplicate_Error e) {
        cerr << "ERROR 1062 (23000): Duplicate entry '" << e.value << "' for key '" << e.attribute << "'" << endl;
    } catch(table_already_exit_error e) {
        cerr << "ERROR 1050 (42S01): Table '" << e.tableName << "' already exists" << endl;
    } catch(Attribute_Not_Exist_Error e) {
        cerr << "ERROR 1054 (42S22): Unknown column '" << e.attriName << "' in 'where clause'" << endl;
    } catch(FileIO_error) {
        cerr << "File Write/Read failed" << endl;
    } catch(name_illegal_error e) {
        cerr << "ERROR 1064 (42000): You have an error in your SQL syntax; '" << e.name << "' isn't an legal name" << endl;
    } catch(string_overflow_error e) {
        cerr << "ERROR 1406 (22001): Data too long for column '" << e.name << "' at row 1" << endl;
    } catch(Index_Not_Exist_Error e) {
        cerr << "ERROR 1091 (42000): Can't DROP '" + e.indexName + "'; check that column/key exists" << endl;
    } catch(Index_Attribute_Not_Unique_Error e) {
        cerr << "ERROR 1091 (42000): Can't create index on '" + e.attribute + "' for it isn't unique attribute." << endl;
    } catch(file_open_error e) {
        cerr << "ERROR:" << endl;
        cerr << "Failed to open file '" + e.fileName + "', error: 2" << endl;
    }
}

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

bool isLegal(const string name)
{
    // 长度小于等于20
    if(name.length() > 20)
        throw name_illegal_error(name);
    // 首字符为字母或者下划线
    if(isalpha(name.front()) || name.front() == '_') {
        // 不能有除了字母、数字和下划线以外的特殊字符
        for(const auto i : name) {
            if(!isalnum(i) && i != '_')
                throw name_illegal_error(name);
        }
    }
    else
        throw name_illegal_error(name);

    return true;
}

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

string Interpreter::parseString(string keyword)
{
    if(keyword.length() < 3 || keyword.front() != keyword.back() || (keyword.front() != '\'' && keyword.front() != '\"')) {
        cout << keyword.front() << endl;
        cout << keyword.back() << endl;
        cout << (keyword.front() != '\"') << endl;
        throw query_formation_error();
    }
    // cout << keyword << " " << string(keyword.begin() + 1, keyword.end() - 1) << endl;
    return string(keyword.begin() + 1, keyword.end() - 1);
}
float Interpreter::parseFloat(string keyword)
{
    // cout << keyword << " " << stof(keyword) << endl;
    return stof(keyword);
}
int Interpreter::parseInt(string keyword)
{
    // cout << keyword << " " << stoi(keyword) << endl;
    return stoi(keyword);
}
#ifndef INDEXSTRING_H
#define INDEXSTRING_H

#include <cstring>
#include <string>
#include <iostream>

using namespace std;

struct IndexString
{
	char content[256];
	friend bool operator<(IndexString s1, IndexString s2);
	friend bool operator<=(IndexString s1, IndexString s2);
	friend bool operator>(IndexString s1, IndexString s2);
	friend bool operator>=(IndexString s1, IndexString s2);
	friend bool operator==(IndexString s1, IndexString s2);
	friend bool operator!=(IndexString s1, IndexString s2);
    friend ostream &operator<<(ostream &os, const IndexString &s);
	IndexString() {
		strcpy(content, "");
	}
	IndexString(const IndexString &s) {
		strcpy(content, s.content);
	}
	IndexString &operator=(const IndexString &s) {
		strcpy(content, s.content);
        return *this;
	}

    IndexString(const string &s) {
        strcpy(content, s.c_str());
    }
};


ostream &operator<<(ostream &os, const IndexString &s);
bool operator<(IndexString s1, IndexString s2);
bool operator<=(IndexString s1, IndexString s2);
bool operator>(IndexString s1, IndexString s2);
bool operator>=(IndexString s1, IndexString s2);
bool operator!=(IndexString s1, IndexString s2);
bool operator==(IndexString s1, IndexString s2);

#endif
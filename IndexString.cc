#include "IndexString.h"

ostream &operator<<(ostream &os, const IndexString &s) {
    os << s.content;
    return os;
}

bool operator<(IndexString s1, IndexString s2)
{
	return (strcmp(s1.content, s2.content) < 0);
}
bool operator<=(IndexString s1, IndexString s2)
{
	return (strcmp(s1.content, s2.content) <= 0);
}
bool operator>(IndexString s1, IndexString s2)
{
	return (strcmp(s1.content, s2.content) > 0);
}
bool operator>=(IndexString s1, IndexString s2)
{
	return (strcmp(s1.content, s2.content) >= 0);
}
bool operator!=(IndexString s1, IndexString s2)
{
	return (strcmp(s1.content, s2.content) != 0);
}
bool operator==(IndexString s1, IndexString s2)
{
	return (strcmp(s1.content, s2.content) == 0);
}
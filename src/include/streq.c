#include "logger.h"
#include <ctype.h>

#ifndef _STR_EQ_H
#define _STR_EQ_H
int str_eq(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        ++s1;
        ++s2;
    }
    return *s1 == *s2;
}

int str_eqi(const char *s1, const char *s2) {
    while (tolower(*s1) == tolower(*s2) && *s1 != '\0') {
        ++s1;
        ++s2;
    }
    return tolower(*s1) == tolower(*s2);
}
#endif /* _STR_EQ_H */
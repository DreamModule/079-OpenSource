#ifndef STRING_H
#define STRING_H

#include "types.h"


int strlen(const char* str);
void strcpy(char* dest, const char* src);
void strncpy(char* dest, const char* src, size_t n);
int strcmp(const char* s1, const char* s2);
int strncmp(const char* s1, const char* s2, size_t n);
void strcat(char* dest, const char* src);


void safe_strcat(char* dest, const char* src, size_t dest_size);


void* memset(void* ptr, int value, size_t num);
void* memcpy(void* dest, const void* src, size_t num);
int memcmp(const void* ptr1, const void* ptr2, size_t num);


void itoa(int value, char* str, int base);
void uitoa(unsigned int value, char* str, int base);

#endif

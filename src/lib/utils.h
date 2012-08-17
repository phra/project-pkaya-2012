#ifndef __UTILS__
#define __UTILS__
void memset(void *ptr, int val, unsigned int len);
int memcmp(void* ptr1, void* ptr2, int lenght);
void memcpy(void* dest, void* src, int lenght);
int strlen(char* str);
int strcmp(char* str1, char* str2);
void strcpy(char* dest, char* src);
void strreverse(char* begin, char* end);
void itoa(int value, char* str, int base);
#endif

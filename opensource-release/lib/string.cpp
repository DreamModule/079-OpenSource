#include "string.h"

int strlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

void strcpy(char* dest, const char* src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void strncpy(char* dest, const char* src, size_t n) {
    size_t i;
    for (i = 0; i < n && src[i] != '\0'; i++) {
        dest[i] = src[i];
    }
    for (; i < n; i++) {
        dest[i] = '\0';
    }
}

int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    while (n && *s1 && (*s1 == *s2)) {
        s1++;
        s2++;
        n--;
    }
    if (n == 0) return 0;
    return *(unsigned char*)s1 - *(unsigned char*)s2;
}

void strcat(char* dest, const char* src) {
    while (*dest) dest++;
    strcpy(dest, src);
}

void safe_strcat(char* dest, const char* src, size_t dest_size) {
    size_t dest_len = strlen(dest);
    if (dest_len >= dest_size - 1) return;  

    size_t i = 0;
    while (src[i] && dest_len + i < dest_size - 1) {
        dest[dest_len + i] = src[i];
        i++;
    }
    dest[dest_len + i] = '\0';
}

void* memset(void* ptr, int value, size_t num) {
    unsigned char* p = (unsigned char*)ptr;
    while (num--) {
        *p++ = (unsigned char)value;
    }
    return ptr;
}

void* memcpy(void* dest, const void* src, size_t num) {
    unsigned char* d = (unsigned char*)dest;
    const unsigned char* s = (const unsigned char*)src;
    while (num--) {
        *d++ = *s++;
    }
    return dest;
}

int memcmp(const void* ptr1, const void* ptr2, size_t num) {
    const unsigned char* p1 = (const unsigned char*)ptr1;
    const unsigned char* p2 = (const unsigned char*)ptr2;
    while (num--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++;
        p2++;
    }
    return 0;
}

void itoa(int value, char* str, int base) {
    
    
    
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    unsigned int uvalue;

    if (value < 0 && base == 10) {
        *ptr++ = '-';
        ptr1++;
        uvalue = (unsigned int)(-(value + 1)) + 1u;  
    } else {
        uvalue = (unsigned int)value;
    }

    do {
        unsigned int quot = uvalue / (unsigned int)base;
        unsigned int rem  = uvalue - quot * (unsigned int)base;
        *ptr++ = "0123456789abcdef"[rem];
        uvalue = quot;
    } while (uvalue);

    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

void uitoa(unsigned int value, char* str, int base) {
    char* ptr = str;
    char* ptr1 = str;
    char tmp_char;
    unsigned int tmp_value;

    do {
        tmp_value = value;
        value /= base;
        *ptr++ = "0123456789abcdef"[tmp_value - value * base];
    } while (value);

    *ptr-- = '\0';
    while (ptr1 < ptr) {
        tmp_char = *ptr;
        *ptr-- = *ptr1;
        *ptr1++ = tmp_char;
    }
}

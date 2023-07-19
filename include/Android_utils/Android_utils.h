//
// Created by 泓清 on 2022/9/3.
//
#ifndef NATIVESURFACE_ANDROID_UTILS_H
#define NATIVESURFACE_ANDROID_UTILS_H
#include <iostream>
using namespace std;
#define MD5_SECRET_LEN_16     (16)
#define MD5_BYTE_STRING_LEN   (4)
extern void* get_func;
void *dlblob(const void *blob, size_t len);
string exec(string command);
int get_android_api_level();
void* get_native_create_transaction();
const char *get_android_mac();
#endif //NATIVESURFACE_ANDROID_UTILS_H
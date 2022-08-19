// MathLibrary.h - Contains declarations of math functions
#pragma once

#ifdef MATHLIBRARY_EXPORTS
#define MATHLIBRARY_API __declspec(dllexport)
#else
#define MATHLIBRARY_API __declspec(dllimport)
#endif
#include <string>


//生成rs文件内容
extern "C" MATHLIBRARY_API void GenerateRS(const char * base64_info, char* const rs_base64);
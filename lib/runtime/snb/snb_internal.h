//
// Created by bknun on 7/14/2023.
//

#ifndef SHARP_SNB_INTERNAL_H
#define SHARP_SNB_INTERNAL_H

typedef void (*bridge_fun)(long handle);
typedef long (*link_proc)(const char* funcName);
typedef short (*lib_init)(void *procs[], int, int);
int initializeLibrary(lib_init linit);

#ifdef _WIN32
#define load_func(handle, name) GetProcAddress(handle, name)
#else
#define load_func(handle, name) dlsym(handle, name)
#endif

#ifdef _WIN32
#define load_lib(lib) LoadLibrary(lib.c_str())
#else
#define load_lib(lib) dlopen(lib.c_str(), RTLD_LAZY)
#endif

#ifdef _WIN32
#define free_lib(handle) FreeLibrary(handle)
#else
#define free_lib(handle) dlclose(handle)
#endif

#endif //SHARP_SNB_INTERNAL_H

#ifndef __COMMON_H__

#define __COMMON_H__

#ifdef DEBUG_DUMP
#define DBG_PRINT(fmt, ...)   fprintf(file, fmt, __VA_ARGS__)
#else
#define DBG_PRINT(fmt, ...)   /* empty */
#endif

#endif // __COMMON_H__

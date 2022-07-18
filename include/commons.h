/*
 *  This file is owned by the Embedded Systems Laboratory of Seoul National University of Science and Technology
 *  as a part of RT-AIDE or the RTOS-Agnostic and Interoperable Development Environment for Real-time Systems
 *
 *  File: commons.h
 *  Author: 2022 Raimarius Delgado
 *  Description: Includes all common functions and definitions
 *
 *
 *
 *
*/
#ifndef __COMMON_H__
#define __COMMON_H__

#include <stdint.h>
#include <ctype.h>
#include <errno.h>
#include <malloc.h>
#include <pthread.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/sysinfo.h>

#define TRUE			1
#define FALSE			0
#define RET_SUCC		0
#define RET_FAIL		-1
#define MAX_BUFFER_SIZE	512

#define NANOSEC_PER_SEC		1000000000
#define MICROSEC_PER_SEC	1000000
#define MILLISEC_PER_SEC	1000

typedef long			LONG;
typedef int				INT;
typedef int64_t			INT64,	*PINT64;
typedef int32_t			INT32,	*PINT32;
typedef int16_t			INT16,	*PINT16;
typedef int8_t			INT8,	*PINT8;

typedef unsigned int	UINT;
typedef uint64_t		UINT64,	*PUINT64;
typedef uint32_t		UINT32,	*PUINT32;
typedef uint16_t		UINT16,	*PUINT16;
typedef uint8_t			UINT8,	*PUINT8;

typedef void			VOID,	*PVOID;
typedef UINT32			DWORD,	*PDWORD;
typedef UINT16			WORD,	*PWORD;
typedef UINT8			BYTE,	*PBYTE;
typedef bool			BOOL,	*PBOOL;
typedef char			CHAR,	*PCHAR;
typedef VOID			(TASKFCN)(PVOID ARG), (*PTASKFCN)(PVOID ARG);
typedef struct tm		TM, *PTM;


#define PASS				(VOID)0

typedef enum _LOWLEVEL_LOG_TYPE
{
	eTRACE = 0,
	eWARN,
	eERROR,
	eINFO,
} LOWLEVEL_LOG_TYPE;

#define __FILENAME__					(strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define AIDE_TRACE(fmt, ...)			write_lowlevel_logger((const PCHAR)__FILENAME__, __LINE__, eTRACE, (PCHAR) fmt, ##__VA_ARGS__)
#define AIDE_ERROR(fmt, ...)			write_lowlevel_logger((const PCHAR)__FILENAME__, __LINE__, eERROR, (PCHAR) fmt,  ##__VA_ARGS__)
#define AIDE_WARN(fmt, ...)				write_lowlevel_logger((const PCHAR)__FILENAME__, __LINE__, eWARN, (PCHAR)fmt,  ##__VA_ARGS__)
#define AIDE_INFO(fmt, ...)				write_lowlevel_logger((const PCHAR)__FILENAME__, __LINE__, eINFO, (PCHAR)fmt,  ##__VA_ARGS__)

#define ZERO_MEMORY(input, sz)			zero_memory(input, sz)		

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

void	init_lowlevel_logger			(BOOL abOnOff);
void	write_lowlevel_logger			(const PCHAR astrFileName, INT anLineNo, LOWLEVEL_LOG_TYPE aeType, PCHAR astrFmt, ...);
void	zero_memory						(PVOID pInput, size_t aulSize);
INT		get_available_cpus				(VOID);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__COMMON_H__



/*
 *  This file is owned by the Embedded Systems Laboratory of Seoul National University of Science and Technology
 *  as a part of RT-AIDE or the RTOS-Agnostic and Interoperable Development Environment for Real-time Systems
 *
 *  File: commons.c
 *  Author: 2022 Raimarius Delgado
 *  Description: Includes all common functions and definitions
 *
 *
 *
 *
*/
#include "commons.h"

#define RST			"\033[0m"
#define BLACK		"\033[30m"				/* Black */
#define RED			"\033[31m"				/* Red */
#define GREEN		"\033[32m"				/* Green */
#define YELLOW		"\033[33m"				/* Yellow */
#define BLUE		"\033[34m"				/* Blue */
#define MAGENTA		"\033[35m"				/* Magenta */
#define CYAN		"\033[36m"				/* Cyan */
#define WHITE		"\033[37m"				/* White */
#define BOLDBLACK   "\033[1m\033[30m"      /* Bold Black */
#define BOLDRED     "\033[1m\033[31m"      /* Bold Red */
#define BOLDGREEN   "\033[1m\033[32m"      /* Bold Green */
#define BOLDYELLOW  "\033[1m\033[33m"      /* Bold Yellow */
#define BOLDBLUE    "\033[1m\033[34m"      /* Bold Blue */
#define BOLDMAGENTA "\033[1m\033[35m"      /* Bold Magenta */
#define BOLDCYAN    "\033[1m\033[36m"      /* Bold Cyan */
#define BOLDWHITE   "\033[1m\033[37m"      /* Bold White */

static BOOL bVerbose = FALSE;

void 
zero_memory(PVOID pInput, size_t aulSize)
{
	memset(pInput, 0, aulSize);
}

void
init_lowlevel_logger(BOOL abOnOff)
{
	bVerbose = abOnOff;
}

INT
get_available_cpus(VOID)
{
	return get_nprocs();
}

void
write_lowlevel_logger(const PCHAR astrFileName, INT anLineNo, LOWLEVEL_LOG_TYPE aeType, PCHAR astrFmt, ...)
{
	if (TRUE == bVerbose)
	{
		CHAR	strBuf[MAX_BUFFER_SIZE] = "";
		va_list strList;
		va_start(strList, astrFmt);
		vsprintf(strBuf, astrFmt, strList);
		va_end(strList);
		
		time_t stTimeNow;
		CHAR strTimeStamp[15];
		time(&stTimeNow);
		PTM stTimeStruct = localtime(&stTimeNow);
		strftime(strTimeStamp, sizeof(strTimeStamp), "<%H:%M:%S>", stTimeStruct);


		switch (aeType)
		{
		case eTRACE:
			printf(RST "%-10s [TRACE] %s [%s:%d]\n", strTimeStamp, strBuf, astrFileName, anLineNo);
			break;
		
		case eWARN:
			printf(BOLDYELLOW "%-10s [WARNING] %s [%s:%d]\n" RST, strTimeStamp, strBuf, astrFileName, anLineNo);
			break;
		case eERROR:
			printf(BOLDRED "%-10s [ERROR] %s [%s:%d]\n" RST, strTimeStamp, strBuf, astrFileName, anLineNo);
			break;
		case eINFO:
			printf(BOLDGREEN "%-10s [INFO] %s\n" RST, strTimeStamp, strBuf);
			break;
		default:
			break;
		}
	}
}
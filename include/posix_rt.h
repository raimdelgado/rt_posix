/*
 *  This file is owned by the Embedded Systems Laboratory of Seoul National University of Science and Technology
 *  as a part of RT-AIDE or the RTOS-Agnostic and Interoperable Development Environment for Real-time Systems
 *  
 *  File: posix_rt.h 
 *  Author: 2022 Raimarius Delgado
 *  Description: Header file for posix_rt.c which is responsible to create real-time tasks and timers using the standard POSIX API
 * 
 * 
 * 
 * 
*/
#ifndef __POSIX_RT_H__
#define __POSIX_RT_H__

#ifndef _GNU_SOURCE // for pthread_setname_np and gettid
#define _GNU_SOURCE
#endif 

#include "commons.h"

#define DEFAULT_STKSIZE	(65536) //64kb
#define MAX_NAME_LENGTH (32)	//32 bytes
#define LIM_PRIORITY_LO		0
#define LIM_PRIORITY_HI		99
#define SET_DEFAULT_STKSZ	0
#define SET_PRIORITY_MED	50
#define SET_TM_NOW			(RTTIME)-99				

typedef UINT64			RTTIME,			*PRTTIME;
typedef pid_t			PID;
typedef cpu_set_t		CPUSET,			*PCPUSET;
typedef pthread_t		PTHREAD,		*PPTHREAD;
typedef	pthread_attr_t	PTHREADATTR,	*PPTHREADATTR;
typedef struct timespec TIMESPEC;

typedef struct _POSIX_TASK
{
	PTHREAD			stThread;
	PTHREADATTR		stThreadAttr;
	PID				nPid;
	
	/* task specifications */
	DWORD			dwStatus;
	INT				nPriority;
	UINT64			ullStackSize;
	BOOL			bRtMode;
	BOOL			bPeriodic;
	CPUSET			stCpuAffinity;
	CHAR			strName[MAX_NAME_LENGTH];
	RTTIME			ullPeriod;
	
	/* timer related (for periodic tasks) */
	TIMESPEC		stDeadline;
	
	/* task function pointer and arguments */
	PTASKFCN		pTaskFcn;
	PVOID			pTaskArg;

	/* use conditional variable to suspend task */
	BOOL			bStartSuspended;
	pthread_cond_t  cvSuspend;
	pthread_mutex_t mtxSuspend;
} POSIX_TASK;

typedef struct _POSIX_TASK_INFO
{
	INT				nPriority;
	BOOL			bRTMode;
	BOOL			bPeriodic;
	CHAR			strName[MAX_NAME_LENGTH];
	pid_t			nPid;
	DWORD			dwStatus;
} POSIX_TASK_INFO;

typedef enum _ePOSIX_STATE_MACHINE
{
	eUnknown = 0x00,
	eInit,
	eReady,
	ePendingStart,
	eWaiting,
	eRunning,
	eSuspended,
	eDead,
} POSIX_STATE_MACHINE;

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/* TASK MANAGEMENT */
INT				create_rt_task		(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, INT anPriority);
INT				spawn_rt_task		(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, INT anPriority, PTASKFCN apEntry, PVOID apArg);
INT				create_nrt_task		(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize);
INT				spawn_nrt_task		(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, PTASKFCN apEntry, PVOID apArg);
INT				get_task_info		(POSIX_TASK* apTask, POSIX_TASK_INFO* apTaskInfo);
INT				set_cpu_affinity	(POSIX_TASK* apTask, INT anCpuNum);
INT				start_task			(POSIX_TASK* apTask, PTASKFCN apEntry, PVOID apArg);
INT				delete_task			(POSIX_TASK* apTask);
INT				suspend_task		(POSIX_TASK* apTask);
INT				resume_task			(POSIX_TASK* apTask);
POSIX_TASK*		get_self			(VOID);

/* TIMER MANAGEMENT */
INT		set_task_period		(POSIX_TASK* apTask, RTTIME aulStartTime, RTTIME aullPeriod);
RTTIME	read_timer			(VOID);
VOID	spin_timer			(RTTIME aullSpinTimeNS);
INT		wait_next_period	(UINT64* apullOverrunsCnt);

/* TIME CONVERSION */
INT		convert_nsecs_to_timespec	(UINT64 aullNanoSecs, TIMESPEC* apTimeSpec);
INT		convert_timespec_to_nsecs	(TIMESPEC astTimeSpec, UINT64* apullNanoSecs);


void	set_test_variable(POSIX_TASK* apTask, int nValue);

/* ETC */
#ifndef gettid
#define gettid() (pid_t)(system_call(SYS_gettid))
#endif // !gettid

#ifndef cpu_relax
#define cpu_relax() __sync_synchronize()
#endif

LONG system_call(LONG alMagicNo);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //__POSIX_RT_H__



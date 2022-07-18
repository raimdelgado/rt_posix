/*
 *  This file is owned by the Embedded Systems Laboratory of Seoul National University of Science and Technology
 *  as a part of RT-AIDE or the RTOS-Agnostic and Interoperable Development Environment for Real-time Systems
 *
 *  File: posix_rt.c
 *  Author: 2022 Raimarius Delgado
 *  Description: create real-time tasks and timers using the standard POSIX API
 *
 *
 *
 *
*/
#include "posix_rt.h"
#include "version.h"
#define CLOCK_TO_USE CLOCK_MONOTONIC

pthread_key_t g_unTaskKey; // create a specific key to identify the created task

VOID _constructor_fcn(void) __attribute__((constructor));
VOID _destructor_fcn(void) __attribute__((destructor));

/*****************************************************************************/
/* Task management*/
/*****************************************************************************/
void handle_signals(int nSigNum)
{
	init_lowlevel_logger(TRUE);
	DBG_INFO("TASK has been signalled [%d]", nSigNum);
}
/*****************************************************************************/
VOID 
_constructor_fcn(void) // initiates the task management pthread_key
{
	init_lowlevel_logger(TRUE);
	if (pthread_key_create(&g_unTaskKey, NULL))
	{
		DBG_ERROR("CONSTRUCTOR: FAILED : Creating Task Key");
		exit(1);
	}
	else
	{
		// print version when loaded
		DBG_INFO("LOADING RT-POSIX v%d.%d.%d.%d [%s]", (INT)VER_MAJOR, (INT)VER_MINOR, (INT)VER_SUB, (INT)VER_PATCH, VER_NAME);
	}

	signal(SIGTERM, handle_signals);
	signal(SIGINT, handle_signals);

	//close this to let users toggle the logger
	init_lowlevel_logger(FALSE); 
}
/*****************************************************************************/
VOID 
_destructor_fcn(void) // deletes the task management pthread_key
{
	// turn on the logger in case it is turned off by the user
	init_lowlevel_logger(TRUE); 
	if (pthread_key_delete(g_unTaskKey))
	{
		DBG_ERROR("DESTRUCTOR: FAILED : Deleting Task Key");
		exit(1);
	}
	else
	{
		DBG_INFO("RT-POSIX HAS BEEN UNLOADED");
	}
}
/*****************************************************************************/
static void 
_set_current_task(POSIX_TASK* apTask)
{
	pthread_setspecific(g_unTaskKey, apTask);
}
/*****************************************************************************/
static POSIX_TASK* 
_get_current_task(void)
{
	return (POSIX_TASK*)pthread_getspecific(g_unTaskKey);
}
/*****************************************************************************/
static POSIX_TASK* 
_get_posix_task_or_self(POSIX_TASK* apTask)
{
	if (apTask != NULL)
		return apTask;

	POSIX_TASK* pTask;
	pTask = _get_current_task();
	if (pTask == NULL)
		DBG_ERROR("FAILED : Get Current Running Task");

	return pTask;
}
/*****************************************************************************/
INT
convert_nsecs_to_timespec(UINT64 aullNanoSecs, TIMESPEC* apTimeSpec)
{
	// converts nanoseconds to timespec
	TIMESPEC stTemp;
	stTemp.tv_sec  = (INT64)aullNanoSecs / NANOSEC_PER_SEC;
	stTemp.tv_nsec = (INT64)aullNanoSecs % NANOSEC_PER_SEC;
	*apTimeSpec = stTemp;

	return RET_SUCC;
}
/*****************************************************************************/
INT
convert_timespec_to_nsecs(TIMESPEC astTimeSpec, UINT64* apullNanoSecs)
{
	// converts timespec to nanoseconds
	UINT64 ullNanoSecs = (UINT64)astTimeSpec.tv_sec * NANOSEC_PER_SEC;
	ullNanoSecs += (UINT64)astTimeSpec.tv_nsec;
	*apullNanoSecs = ullNanoSecs;

	return RET_SUCC;
}
/*****************************************************************************/
PVOID 
default_trampoline_proc(PVOID arg)
{
	POSIX_TASK* pTask = (POSIX_TASK*)arg;
	
	if (pTask == NULL || (pTask->dwStatus != ePendingStart && pTask->dwStatus != eSuspended))
	{
		DBG_ERROR("FAILED : START PROC : pTask is NULL or pTask is not Started!");
		exit(-1);
	}
	else if ((pTask->dwStatus == eSuspended) || (pTask->bStartSuspended == TRUE))
	{
		DBG_TRACE("START PROC : %s Task Start Suspended! Waiting for resume_task()", pTask->strName);
		pthread_mutex_lock(&pTask->mtxSuspend);
		pTask->dwStatus = (DWORD)eSuspended;
		int nRet = pthread_cond_wait(&pTask->cvSuspend, &pTask->mtxSuspend);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : START PROC (pthread_cond_wait): %s with errno (%d:%s)", pTask->strName, nRet, strerror(nRet));
			exit(-1);
		}
		pthread_mutex_unlock(&pTask->mtxSuspend);
		pTask->bStartSuspended = FALSE;
	}
	
	if((pthread_setname_np(pTask->stThread, pTask->strName)))
		DBG_WARN("WARNING : START PROC (pthread_setname_np): %s", pTask->strName);

	pTask->nPid = gettid();
	_set_current_task(pTask);
	DBG_TRACE("START PROC : %s Task Started! (PID: %d)", pTask->strName, pTask->nPid);
	pTask->dwStatus = (DWORD)eRunning;
	
	// run the function pointer (entry of the task)
	pTask->pTaskFcn(pTask->pTaskArg);
	
	pTask->dwStatus = (DWORD)eDead;
	DBG_TRACE("START PROC : %s Task Ended!", pTask->strName);
	return NULL;
}
/*****************************************************************************/
static void 
_init_posix_task(POSIX_TASK* apTask)
{

	apTask->nPid = 0;
	apTask->dwStatus = (DWORD)eInit;
	apTask->nPriority = 0;
	apTask->ullStackSize = DEFAULT_STKSIZE;
	apTask->bRtMode = FALSE;
	apTask->bPeriodic = FALSE;

	/* first CPU core as the default */
	CPU_ZERO(&apTask->stCpuAffinity);
	CPU_SET(0, &apTask->stCpuAffinity);
	
	/* Clear strName */
	ZERO_MEMORY(apTask->strName, sizeof(apTask->strName));
	
	apTask->ullPeriod = 0;
	apTask->stDeadline.tv_sec = 0;
	apTask->stDeadline.tv_nsec= 0;

	apTask->pTaskFcn = NULL;
	apTask->pTaskArg = NULL;

	apTask->bStartSuspended = FALSE;
}
/*****************************************************************************/
static INT 
_create_task(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, INT anPriority, BOOL abIsTaskRT)
{
	int nRet = RET_FAIL;

	if (strlen(astrName) > MAX_NAME_LENGTH)
	{
		DBG_ERROR("FAILED : Create TASK (Length of astrName should be less than %d)", (INT)MAX_NAME_LENGTH);
		return -EINVAL;
	}
	
	_init_posix_task(apTask);
	// setup task name and RT mode
	strcpy(apTask->strName, astrName);
	apTask->bRtMode = abIsTaskRT;
	BOOL bIsRealTime = apTask->bRtMode;

	// initiate pthread attributes
	nRet = pthread_attr_init(&apTask->stThreadAttr);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Create TASK (pthread_attr_init): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
		return -nRet;
	}

	// create pthread as detached so that its thread ID and other resources can be reused as soon as the thread terminates
	nRet = pthread_attr_setdetachstate(&apTask->stThreadAttr, PTHREAD_CREATE_DETACHED);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Create TASK (pthread_attr_setdetachstate): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
		return -nRet;
	}

	// start of configuration required for real-time tasks
	if (bIsRealTime == TRUE)
	{
		// ensure that the scheduler of the created thread is not inherited from the creating (usually main) thread
		nRet = pthread_attr_setinheritsched(&apTask->stThreadAttr, PTHREAD_EXPLICIT_SCHED);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Create TASK (pthread_attr_setinheritsched): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
			return -nRet;
		}
		// default scheduler is SCHED_OTHER which is a time-sharing scheduler.
		// we need SCHED_FIFO to implement priority-based pre-emptive scheduling.
		nRet = pthread_attr_setschedpolicy(&apTask->stThreadAttr, SCHED_FIFO);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Create TASK (pthread_attr_setschedpolicy): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
			return -nRet;
		}
		// implement priority limits
		if (anPriority > (INT)LIM_PRIORITY_LO && anPriority <= (INT)LIM_PRIORITY_HI)
			apTask->nPriority = anPriority;
		else
		{
			DBG_ERROR("FAILED : Create TASK (anPriority should be within the range of %d ~ %d)", (INT)LIM_PRIORITY_LO, (INT)LIM_PRIORITY_HI);
			return -EINVAL;
		}
		// set priority of the thread
		struct sched_param stParam = { .sched_priority = apTask->nPriority};
		nRet = pthread_attr_setschedparam(&apTask->stThreadAttr, &stParam);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Create TASK (pthread_attr_setschedparam): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
			return -nRet;
		}
	}
	
	// deploy the thread to CPU0 by default, this can be changed before the tasks starts
	nRet = set_cpu_affinity(apTask, 0);
	if (nRet != RET_SUCC)
		return -nRet;

	// check sanity of stack size
	if (anStkSize < (INT)PTHREAD_STACK_MIN || anStkSize == (INT)SET_DEFAULT_STKSZ)
		apTask->ullStackSize = (UINT64)DEFAULT_STKSIZE;
	else
		apTask->ullStackSize = (UINT64)anStkSize;

	nRet = pthread_attr_setstacksize(&apTask->stThreadAttr, apTask->ullStackSize);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Create TASK (pthread_attr_setstacksize): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
		return -nRet;
	}
	
	apTask->dwStatus = (DWORD)eReady;

	/* initiate conditional variable and corresponding mutex */
	nRet = pthread_cond_init(&apTask->cvSuspend, NULL);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Create TASK (pthread_cond_init): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
		return -nRet;
	}
	
	pthread_mutexattr_t *pstMtxAttr = malloc(sizeof(pthread_mutexattr_t));
	// we need to ensure that the mutex implements priority inheritance in case of real-time task
	if (bIsRealTime == TRUE)
	{
		pthread_mutexattr_t stMtxAttr;
		nRet = pthread_mutexattr_init(&stMtxAttr);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Create TASK (pthread_mutexattr_init): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
			return -nRet;
		}
		nRet = pthread_mutexattr_setprotocol(&stMtxAttr, PTHREAD_PRIO_INHERIT);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Create TASK (pthread_mutexattr_init): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
			return -nRet;
		}
		pstMtxAttr = &stMtxAttr;
	}
	else pstMtxAttr = NULL;
		
	nRet = pthread_mutex_init(&apTask->mtxSuspend, pstMtxAttr);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Create TASK (pthread_mutexattr_init): %s with errno (%d:%s)", astrName, nRet, strerror(nRet));
		return -nRet;
	}

	return RET_SUCC;
}
/*****************************************************************************/
INT 
create_rt_task(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, INT anPriority)
{
	INT nRet = _create_task(apTask, astrName, anStkSize, anPriority, TRUE);
	if (nRet == RET_SUCC)
		DBG_TRACE("SUCCESS: Create RT TASK : name=%s, priority=%d", apTask->strName, apTask->nPriority);
	else
		DBG_ERROR("FAILED : Create RT TASK: %s with errno (%d:%s)",astrName, nRet, strerror(-nRet));

	return nRet;
}
/*****************************************************************************/
INT		
spawn_rt_task(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, INT anPriority, PTASKFCN apEntry, PVOID apArg)
{
	INT nRet = create_rt_task(apTask, astrName, anStkSize, anPriority);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Spawn RT TASK: could not CREATE %s with errno (%d:%s)", astrName, nRet, strerror(-nRet));
		return nRet;
	}
	nRet = start_task(apTask, apEntry, apArg);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Spawn RT TASK: could not START %s with errno (%d:%s)", astrName, nRet, strerror(-nRet));
		return nRet;
	}

	DBG_TRACE("SUCCESS: Spawn RT TASK : name=%s, priority=%d", apTask->strName, apTask->nPriority);
	return RET_SUCC;
}
/*****************************************************************************/
INT
create_nrt_task(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize)
{
	INT nRet = _create_task(apTask, astrName, anStkSize, 0, FALSE);
	if (nRet == RET_SUCC)
		DBG_TRACE("SUCCESS: Create NRT TASK: name=%s", apTask->strName);
	else
		DBG_ERROR("FAILED : Create NRT TASK: %s with errno (%d:%s)", astrName, nRet, strerror(-nRet));

	return nRet;
}
/*****************************************************************************/
INT
spawn_nrt_task(POSIX_TASK* apTask, const PCHAR astrName, INT anStkSize, PTASKFCN apEntry, PVOID apArg)
{
	INT nRet = create_nrt_task(apTask, astrName, anStkSize);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Spawn NRT TASK: could not CREATE %s with errno (%d:%s)", astrName, nRet, strerror(-nRet));
		return nRet;
	}
	nRet = start_task(apTask, apEntry, apArg);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Spawn NRT TASK: could not START %s with errno (%d:%s)", astrName, nRet, strerror(-nRet));
		return nRet;
	}
	
	DBG_TRACE("SUCCESS: Spawn NRT TASK : name=%s", apTask->strName);
	return RET_SUCC;
}
/*****************************************************************************/
INT
start_task(POSIX_TASK* apTask, PTASKFCN apEntry, PVOID apArg)
{
	INT nRet = RET_FAIL;

	if (apTask == NULL || apTask->dwStatus > eReady)
	{
		DBG_ERROR("FAILED : START TASK: apTask is either NULL or has already started!");
		return -EWOULDBLOCK;
	}
	DWORD dwPreviousStatus = apTask->dwStatus;
	apTask->pTaskFcn = apEntry;
	apTask->pTaskArg = apArg;
	apTask->dwStatus = (DWORD)ePendingStart;

	nRet = pthread_create(&apTask->stThread, &apTask->stThreadAttr, default_trampoline_proc, apTask);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : START TASK (pthread_create): %s with errno (%d:%s)", apTask->strName, nRet, strerror(nRet));
		apTask->dwStatus = dwPreviousStatus;
		return -nRet;
	}

	_set_current_task(apTask);
	nRet = pthread_attr_destroy(&apTask->stThreadAttr);
	if (nRet != RET_SUCC)
	{
		DBG_WARN("WARNING : START TASK (pthread_attr_destroy): %s with errno (%d:%s)", apTask->strName, nRet, strerror(nRet));
	}

	return RET_SUCC;
}
/*****************************************************************************/
INT
delete_task(POSIX_TASK* apTask)
{
	INT nRet = RET_FAIL;
	POSIX_TASK* pTask;
	
	pTask = _get_posix_task_or_self(apTask);
	if (pTask == NULL)
	{
		DBG_ERROR("FAILED : Delete Posix TASK: with errno (%d:%s)", EPERM, strerror(EPERM));
		return -EPERM;
	}
	// return immediately if task is either dead or suspended
	if (pTask->dwStatus >= eDead)
	{
		nRet = RET_SUCC;
	}
	// 
	else if (pTask->dwStatus <= eReady)
	{
		// reset all task settings if the task has not been started yet
		_init_posix_task(apTask);
		nRet = RET_SUCC;
	}
	else
	{
		PID nPid = pTask->nPid;
		// kill the thread forcibly
		nRet = kill(nPid, -SIGKILL);
	}
	return nRet;
}
/*****************************************************************************/
INT
suspend_task(POSIX_TASK* apTask)
{
	INT nRet = RET_FAIL;
	POSIX_TASK* pTask;
	
	pTask = _get_posix_task_or_self(apTask);
	if (pTask == NULL)
	{
		DBG_ERROR("FAILED : Suspend Posix TASK: with errno (%d:%s)", EPERM, strerror(EPERM));
		return -EPERM;
	}
	// return immediately if task is either dead or suspended
	if (pTask->dwStatus >= eSuspended)
	{
		nRet = RET_SUCC;
	}
	// 
	else if (pTask->dwStatus <= ePendingStart)
	{
		pTask->bStartSuspended = TRUE;
		nRet = RET_SUCC;
	}
	else
	{
		pthread_mutex_lock(&pTask->mtxSuspend);
		pTask->dwStatus = (DWORD)eSuspended;
		nRet = pthread_cond_wait(&pTask->cvSuspend, &pTask->mtxSuspend);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Suspend Task (pthread_cond_wait): %s with errno (%d:%s)", pTask->strName, nRet, strerror(nRet));
			exit(-1);
		}
		pthread_mutex_unlock(&pTask->mtxSuspend);
	}
	return nRet;
}
/*****************************************************************************/
INT
resume_task(POSIX_TASK* apTask)
{
	POSIX_TASK* pTask;
	pTask = _get_posix_task_or_self(apTask);
	if (pTask == NULL)
	{
		DBG_ERROR("FAILED : Resume Posix TASK: with errno (%d:%s)", EPERM, strerror(EPERM));
		return -EPERM;
	}
	if (pTask->dwStatus == eSuspended)
	{
		int nRet = pthread_cond_signal(&pTask->cvSuspend);
		if (nRet != RET_SUCC)
		{
			DBG_ERROR("FAILED : Resume Task (pthread_cond_wait): %s with errno (%d:%s)", pTask->strName, nRet, strerror(nRet));
			exit(-1);
		}
	}
	return RET_SUCC;
}
/*****************************************************************************/
INT
get_task_info(POSIX_TASK* apTask, POSIX_TASK_INFO* apTaskInfo)
{
	POSIX_TASK* pTask;
	pTask = _get_posix_task_or_self(apTask);
	if (pTask == NULL)
		return -EPERM;

	if (apTaskInfo == NULL)
		return -EPERM;

	apTaskInfo->nPriority = pTask->nPriority;
	apTaskInfo->bRTMode = pTask->bRtMode;
	apTaskInfo->bPeriodic = pTask->bPeriodic;
	memcpy(apTaskInfo->strName, pTask->strName, sizeof(pTask->strName));
	apTaskInfo->nPid = pTask->nPid;
	apTaskInfo->dwStatus = pTask->dwStatus;
	
	return RET_SUCC;
}
/*****************************************************************************/
INT		
set_cpu_affinity(POSIX_TASK* apTask, INT anCpuNum)
{
	if (apTask == NULL)
		return -EPERM;
	
	// disallow changing the affinity when the task is already scheduled
	if (apTask->dwStatus > eReady)
	{
		DBG_ERROR("FAILED : Set CPU Affinity: This should be called before starting the RT Task!");
		return -EPERM;
	}
	
	int nCpuNum = 0;
	// ensure that the anCpuNum is within the range of available CPUs
	if (anCpuNum > (get_available_cpus()-1))
	{
		DBG_ERROR("FAILED : Set CPU Affinity: anCpuNum is greater than the available CPUS!");
		return -EINVAL;
	}
	else if (anCpuNum < 0)  nCpuNum = 0;
	else nCpuNum = anCpuNum;
	
	CPU_ZERO(&apTask->stCpuAffinity);
	CPU_SET((size_t)nCpuNum, &apTask->stCpuAffinity);

	int nRet = pthread_attr_setaffinity_np(&apTask->stThreadAttr, sizeof(CPUSET), &apTask->stCpuAffinity);
	if (nRet != RET_SUCC)
	{
		DBG_ERROR("FAILED : Set CPU Affinity: %s with errno (%d:%s)", apTask->strName, nRet, strerror(nRet));
		return -nRet;
	}

	DBG_TRACE("SUCCESS: Set CPU Affinity: taskname=%s, cpu#=%d",apTask->strName, nCpuNum);
	return RET_SUCC;
}
/*****************************************************************************/
POSIX_TASK*
get_self(VOID)
{
	POSIX_TASK* pTask;
	pTask = _get_current_task();
	if (pTask == NULL)
		DBG_WARN("WARNING : Get Self is not called inside a POSIX task... returning NULL");

	return pTask;
}
/*****************************************************************************/
INT		
set_task_period(POSIX_TASK* apTask, RTTIME aulStartTime, RTTIME aullPeriod)
{
	INT nRet = RET_FAIL;

	TIMESPEC stStartTime;
	POSIX_TASK* pTask;

	pTask = _get_posix_task_or_self(apTask);
	if (pTask == NULL || pTask->dwStatus > ePendingStart)
		goto failure;

	if (aulStartTime == (RTTIME)SET_TM_NOW)
		nRet = clock_gettime(CLOCK_TO_USE, &stStartTime);
	else
		nRet = convert_nsecs_to_timespec(aulStartTime, &stStartTime);

	if (nRet != RET_SUCC)
		goto failure;

	/* add period to start_time */
	stStartTime.tv_nsec += (INT64) aullPeriod;
	stStartTime.tv_sec += stStartTime.tv_nsec / NANOSEC_PER_SEC;
	stStartTime.tv_nsec %= NANOSEC_PER_SEC;

	pTask->stDeadline = stStartTime;
	pTask->ullPeriod = aullPeriod;
	pTask->bPeriodic = TRUE;

	return RET_SUCC;

failure:
	DBG_ERROR("FAILED : Set Task Period: Invalid task parameters");
	return -EWOULDBLOCK;
}
/*****************************************************************************/
RTTIME	
read_timer(VOID)
{
	TIMESPEC	stNow;
	if (clock_gettime(CLOCK_TO_USE, &stNow))
	{
		DBG_ERROR("FAILED : Read Timer!");
		return (RTTIME)RET_FAIL;
	}
	else
	{
		RTTIME		rttNow;
		convert_timespec_to_nsecs(stNow, &rttNow);
		return rttNow;
	}
}
/*****************************************************************************/
VOID	
spin_timer(RTTIME aullSpinTimeNS)
{
	RTTIME rttEndTime;
	rttEndTime = read_timer() + aullSpinTimeNS;
	while (read_timer() < rttEndTime)
		cpu_relax();
}
/*****************************************************************************/
INT		
wait_next_period(UINT64* apullOverrunsCnt)
{
	POSIX_TASK* pTask;

	pTask = _get_current_task();
	if (pTask == NULL || pTask->bPeriodic == FALSE)
		return -EWOULDBLOCK;

	TIMESPEC stNow;
	pTask->dwStatus = (DWORD)eWaiting;
	INT nRet = clock_nanosleep(CLOCK_TO_USE, TIMER_ABSTIME, &pTask->stDeadline, NULL);
	if (nRet != RET_SUCC)
	{
		DBG_WARN("WARNING : WAIT NEXT PERIOD : %s with errno (%d:%s)", pTask->strName, nRet, strerror(nRet));
		DBG_WARN("WARNING : WAIT NEXT PERIOD : Continue calculating next period... ");
	}
	else
		pTask->dwStatus = (DWORD)eReady;
	
	// update next deadline
	pTask->stDeadline.tv_nsec += (INT64)pTask->ullPeriod;
	pTask->stDeadline.tv_sec += pTask->stDeadline.tv_nsec / NANOSEC_PER_SEC;
	pTask->stDeadline.tv_nsec %= NANOSEC_PER_SEC;
	
	// check for missed deadlines
	convert_nsecs_to_timespec(read_timer(), &stNow);
	if ((stNow.tv_sec > pTask->stDeadline.tv_sec) || (stNow.tv_sec == pTask->stDeadline.tv_sec && pTask->stDeadline.tv_nsec > stNow.tv_nsec))
	{
		if (apullOverrunsCnt != NULL)
		{
			*apullOverrunsCnt += 1;
			DBG_WARN("WARNING : WAIT NEXT PERIOD : %s overrun occurs cnt=%d ", pTask->strName, *apullOverrunsCnt);
		}
		else
		{
			DBG_WARN("WARNING : WAIT NEXT PERIOD : %s overrun occurs", pTask->strName);
		}
		nRet = -ETIMEDOUT;
	}
	else
	{
		if (apullOverrunsCnt != NULL)
			*apullOverrunsCnt = 0;
		
		nRet = RET_SUCC;
	}
	return nRet;
}
/*****************************************************************************/
LONG
system_call(LONG alMagicNo)
{
	return syscall(alMagicNo);
}
/*****************************************************************************/
#include <stdio.h>
#include "posix_rt.h"
#include <string.h>


POSIX_TASK g_stRtTask1;
POSIX_TASK g_stRtTask2;

void sample_periodic_proc(void* arg)
{
    int nCnt = 0;
    while (1)
    {
        wait_next_period(NULL);
	
	if (0 == (nCnt % 1000))
        	printf("PERIODIC TASK: %d\n", nCnt);

	nCnt++;
    }
}

void sample_proc(void* arg)
{
    int *nRet = (int*) arg;
    printf("ONESHOT TASK\n");
    *nRet = 55;
}

void signal_handler(int nSignal)
{
    init_lowlevel_logger(TRUE);
    DBG_INFO("RT-POSIX has been signalled [%d]", nSignal);
    exit(0);
}

int main()
{

    signal(SIGTERM, signal_handler);
    signal(SIGINT, signal_handler);
    
    /* Lock all current and future pages from preventing of being paged to swap */
    mlockall(MCL_CURRENT | MCL_FUTURE);

    int nRet = 0;
    init_lowlevel_logger(TRUE);
    create_rt_task(&g_stRtTask1, (const PCHAR)"PERIODIC", 0, 99);
    set_task_period(&g_stRtTask1, SET_TM_NOW, 1000000);
    
    create_rt_task(&g_stRtTask2, (const PCHAR)"ONESHOT", 0, 80);
    
    start_task(&g_stRtTask1, &sample_periodic_proc, NULL);
    start_task(&g_stRtTask2, &sample_proc, (void*) &nRet);
    
    // periodic task will run only twice
    pause();
    //sleep(2);
    
    // check if the value from the ONESHOT thread has been acquired successfully
    printf("nRet: %d\n", nRet);

    return RET_SUCC;
}



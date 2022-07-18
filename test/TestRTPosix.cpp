/*
 *  This file is owned by the Embedded Systems Laboratory of Seoul National University of Science and Technology
 *  as a part of RT-AIDE or the RTOS-Agnostic and Interoperable Development Environment for Real-time Systems
 *
 *  File: TestRTPosix.cpp
 *  Author: 2022 Raimarius Delgado
 *  Description: Unit test for the RT-Posix Library based on GTest
 *
 *
 *
 *
*/
#include "UnitTest.h"
#include "posix_rt.h"

TEST(testRTPOSIX, create_rt_task)
{
    POSIX_TASK stTask;
    INT nRet = 0;

    // astrName is greater than the limit
    nRet = create_rt_task(&stTask, (const PCHAR)"ABCDEFGHIJKLMNOPQABCDEFGHIJKLMNOPQ", 0, 99);
    EXPECT_EQ(-EINVAL, nRet);

    // priority is greater than LIM_PRIORITY_HIGH (99)
    nRet = create_rt_task(&stTask, (const PCHAR)"ABCD", 0, 100);
    EXPECT_EQ(-EINVAL, nRet);

    // priority is lower than LIM_PRIORITY_LOW+1 (0 + 1)
    nRet = create_rt_task(&stTask, (const PCHAR)"ABCD", 0, 0);
    EXPECT_EQ(-EINVAL, nRet);

    // Successful
    nRet = create_rt_task(&stTask, (const PCHAR)"ABCD", 0, 99);
    EXPECT_EQ(RET_SUCC, nRet);
}

TEST(testRTPOSIX, create_nrt_task)
{
    POSIX_TASK stTask;
    INT nRet = 0;

    // astrName is greater than the limit
    nRet = create_nrt_task(&stTask, (const PCHAR)"ABCDEFGHIJKLMNOPQABCDEFGHIJKLMNOPQ", 0);
    EXPECT_EQ(-EINVAL, nRet);

    nRet = create_nrt_task(&stTask, (const PCHAR)"ABCD", 0);
    EXPECT_EQ(RET_SUCC, nRet);
}

void test_oneshot_proc(void* arg)
{
    int *nArg = (int*)arg;
    *nArg = 55; // return this after the thread finishes
}

TEST(testRTPOSIX, start_task)
{
    POSIX_TASK stRTTask;
    POSIX_TASK stNRTTask;
    INT nRet = 0;
    INT nArg = 0;
    
    // Create Task Successfully
    nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, 99);
    EXPECT_EQ(RET_SUCC, nRet);

    nRet = create_nrt_task(&stNRTTask, (const PCHAR)"ABCDE", 0);
    EXPECT_EQ(RET_SUCC, nRet);

    // Start Task Fail
    nRet = start_task(NULL, &test_oneshot_proc, (void*)&nArg);
    EXPECT_EQ(-EWOULDBLOCK, nRet);

    // Start Task Successful
    nRet = start_task(&stRTTask, &test_oneshot_proc, (void*)&nArg);
    EXPECT_EQ(RET_SUCC, nRet);
    sleep(2);
    EXPECT_EQ(55, nArg);

    // Start Task Successful
    nRet = start_task(&stNRTTask, &test_oneshot_proc, (void*)&nArg);
    EXPECT_EQ(RET_SUCC, nRet);
    sleep(2);
    EXPECT_EQ(55, nArg);
}

TEST(testRTPOSIX, spawn_rt_task)
{
    POSIX_TASK stRTTask;
    INT nArg = 0, nRet = 0;

    // Create Task Successfully
    nRet = spawn_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, 99, &test_oneshot_proc, (void*)&nArg);
    EXPECT_EQ(RET_SUCC, nRet);
    sleep(2);
    EXPECT_EQ(55, nArg);
}

TEST(testRTPOSIX, spawn_nrt_task)
{
    POSIX_TASK stRTTask;
    INT nArg = 0, nRet = 0;

    // Create Task Successfully
    nRet = spawn_nrt_task(&stRTTask, (const PCHAR)"ABCD", 0, &test_oneshot_proc, (void*)&nArg);
    EXPECT_EQ(RET_SUCC, nRet);
    sleep(2);
    EXPECT_EQ(55, nArg);
}

TEST(testRTPOSIX, get_task_info)
{
    POSIX_TASK stRTTask;
    POSIX_TASK_INFO stTaskInfo;
    INT nRet = 0;
    
    // Create Task Successfully
    nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, 99);
    EXPECT_EQ(RET_SUCC, nRet);
    
    // return stRTTask
    nRet = get_task_info(NULL, &stTaskInfo);
    EXPECT_EQ(RET_SUCC, nRet);

    // failed
    nRet = get_task_info(&stRTTask, NULL);
    EXPECT_EQ(-EPERM, nRet);

    // return stRTTask
    nRet = get_task_info(&stRTTask, &stTaskInfo);
    EXPECT_EQ(RET_SUCC, nRet);

    EXPECT_EQ(stRTTask.nPriority, stTaskInfo.nPriority);
    EXPECT_EQ(stRTTask.bRtMode, stTaskInfo.bRTMode);
    EXPECT_EQ(stRTTask.bPeriodic, stTaskInfo.bPeriodic);
    EXPECT_STREQ(stRTTask.strName, stTaskInfo.strName);
    EXPECT_EQ(stRTTask.nPid, stTaskInfo.nPid);
    EXPECT_EQ(stRTTask.dwStatus, stTaskInfo.dwStatus);
}

TEST(testRTPOSIX, set_cpu_affinity)
{
    POSIX_TASK stRTTask;
    INT nRet = set_cpu_affinity(NULL, 0);
    EXPECT_EQ(-EPERM, nRet);

    // Create Task Successfully
    nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, 99);
    EXPECT_EQ(RET_SUCC, nRet);

    nRet = set_cpu_affinity(&stRTTask, 99);
    EXPECT_EQ(-EINVAL, nRet);
    
    nRet = set_cpu_affinity(&stRTTask, 0);
    EXPECT_EQ(RET_SUCC, nRet);
}

TEST(testRTPOSIX, delete_task)
{
    POSIX_TASK stRTTask;
    INT nPriority = 99;
    
    INT nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, nPriority);
    EXPECT_EQ(RET_SUCC, nRet);
    EXPECT_EQ(nPriority, stRTTask.nPriority);

    nRet = delete_task(&stRTTask);
    EXPECT_EQ(RET_SUCC, nRet);
    EXPECT_EQ(0, stRTTask.nPriority);

    stRTTask.dwStatus = eDead;
    nRet = delete_task(&stRTTask);
    EXPECT_EQ(RET_SUCC, nRet);
    EXPECT_EQ(0, stRTTask.nPriority);

}

TEST(testRTPOSIX, suspend_task)
{
    POSIX_TASK stRTTask;
    INT nPriority = 99;

    INT nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, nPriority);
    EXPECT_EQ(RET_SUCC, nRet);
    EXPECT_EQ(nPriority, stRTTask.nPriority);

    stRTTask.dwStatus = eSuspended;
    nRet = suspend_task(&stRTTask);
    EXPECT_EQ(RET_SUCC, nRet);

    stRTTask.dwStatus = ePendingStart;
    nRet = suspend_task(&stRTTask);
    EXPECT_EQ(RET_SUCC, nRet);
}

TEST(testRTPOSIX, resume_task)
{
    POSIX_TASK stRTTask;
    INT nPriority = 99;

    INT nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, nPriority);
    EXPECT_EQ(RET_SUCC, nRet);
    EXPECT_EQ(nPriority, stRTTask.nPriority);

    nRet = resume_task(&stRTTask);
    EXPECT_EQ(RET_SUCC, nRet);
}

TEST(testRTPOSIX, get_self)
{
    POSIX_TASK stRTTask;

    // Create Task Successfully
    INT nRet = create_rt_task(&stRTTask, (const PCHAR)"ABCD", 0, 99);
    EXPECT_EQ(RET_SUCC, nRet);

    POSIX_TASK* pTask = get_self();
    EXPECT_TRUE(pTask==&stRTTask);
}

TEST(testRTPOSIX, spin_timer)
{
    RTTIME nTimerStart = read_timer();
    spin_timer(1000000000);
    RTTIME nTimerDone = read_timer();
    EXPECT_TRUE(nTimerDone >= nTimerStart + 1000000000);
}

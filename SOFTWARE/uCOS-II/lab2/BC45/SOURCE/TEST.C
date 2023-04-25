/*
*********************************************************************************************************
*                                                uC/OS-II
*                                          The Real-Time Kernel
*
*                           (c) Copyright 1992-2002, Jean J. Labrosse, Weston, FL
*                                           All Rights Reserved
*
*                                               EXAMPLE #1
*********************************************************************************************************
*/

#include "includes.h"

/*
*********************************************************************************************************
*                                               CONSTANTS
*********************************************************************************************************
*/

#define  TASK_STK_SIZE                 512       /* Size of each task's stacks (# of WORDs)            */
#define  N_TASKS                        3       /* Number of identical tasks                          */

/*
*********************************************************************************************************
*                                               VARIABLES
*********************************************************************************************************
*/
typedef struct TASK_ARGS                               /* Arguments of real-time task                   */
{
    INT32U     compTime;
    INT32U     period;
    INT32U     deadline;
} TASK_ARGS;

OS_STK        TaskStk[N_TASKS][TASK_STK_SIZE];        /* Tasks stacks                                  */
OS_STK        TaskStartStk[TASK_STK_SIZE];
TASK_ARGS      TaskArgs[N_TASKS];                      /* Parameters to pass to each task               */
OS_EVENT     *RandomSem;

/*
*********************************************************************************************************
*                                           FUNCTION PROTOTYPES
*********************************************************************************************************
*/

        void  Task(void *data);                       /* Function prototypes of tasks                  */
        void  TaskStart(void *data);                  /* Function prototypes of Startup task           */
static  void  TaskStartCreateTasks(void);
static  void  TaskStartDispInit(void);
static  void  TaskStartDisp(void);
        void  Print(void);
/*$PAGE*/
/*
*********************************************************************************************************
*                                                MAIN
*********************************************************************************************************
*/

void  main (void)
{
    // PC_DispClrScr(DISP_FGND_WHITE + DISP_BGND_BLACK);      /* Clear the screen                         */

    OSInit();                                              /* Initialize uC/OS-II                      */

    PC_DOSSaveReturn();                                    /* Save environment to return to DOS        */
    PC_VectSet(uCOS, OSCtxSw);                             /* Install uC/OS-II's context switch vector */

    // RandomSem   = OSSemCreate(1);                          /* Random number semaphore                  */

    OSTaskCreate(TaskStart, (void *)0, &TaskStartStk[TASK_STK_SIZE - 1], 0);

    OSStart();                                             /* Start multitasking                       */
}


/*
*********************************************************************************************************
*                                              STARTUP TASK
*********************************************************************************************************
*/
void  TaskStart (void *pdata)
{
#if OS_CRITICAL_METHOD == 3                                /* Allocate storage for CPU status register */
    OS_CPU_SR  cpu_sr;
#endif
    // char       s[100];
    INT16S     key;

    pdata = pdata;                                         /* Prevent compiler warning                 */

    OS_ENTER_CRITICAL();
    PC_VectSet(0x08, OSTickISR);                           /* Install uC/OS-II's clock tick ISR        */
    PC_SetTickRate(OS_TICKS_PER_SEC);                      /* Reprogram tick rate                      */
    OS_EXIT_CRITICAL();

    OSStatInit();                                          /* Initialize uC/OS-II's statistics         */

    TaskStartCreateTasks();                                /* Create all the application tasks         */
    OSTimeSet(1);
    for (;;) {
        if (PC_GetKey(&key) == TRUE) {                     /* See if key has been pressed              */
            if (key == 0x1B) {                             /* Yes, see if it's the ESCAPE key          */
                PC_DOSReturn();                            /* Return to DOS                            */
            }
        }

        OSCtxSwCtr = 0;                                    /* Clear context switch counter             */
        OSTimeDlyHMSM(0, 0, 1, 0);                         /* Wait one second                          */
        // OSTimeDly(100);
    }
}

/*$PAGE*/
/*
*********************************************************************************************************
*                                             CREATE TASKS
*********************************************************************************************************
*/

static  void  TaskStartCreateTasks (void)
{
    INT8U  i;
    #if N_TASKS == 2
        TaskArgs[0].compTime = 1;                              /* Testcase 1 = {(1, 3), (3, 5)}            */
        TaskArgs[0].period   = 3;
        TaskArgs[0].deadline = 3;

        TaskArgs[1].compTime = 3;
        TaskArgs[1].period   = 5;                              
        TaskArgs[1].deadline = 5;
    #endif

    #if N_TASKS == 3                                       
        TaskArgs[0].compTime = 1;                              /* Testcase 2 = {(1, 4), (2, 5), {2, 10}}   */
        TaskArgs[0].period   = 4;
        TaskArgs[0].deadline = 4;

        TaskArgs[1].compTime = 2;
        TaskArgs[1].period   = 5;
        TaskArgs[1].deadline = 5;

        TaskArgs[2].compTime = 2;
        TaskArgs[2].period   = 10;
        TaskArgs[2].deadline = 10;
    #endif


    row_size = N_TASKS + 3;                                /* Initialize the buffer including STAT, IDLE   */
    col_size = 4;
    pos = 0;
    print_pos = 0;
    buf = (int**)malloc(sizeof(int*)*row_size);
    for(i=0; i<row_size; i++)
        buf[i] = (int*)malloc(sizeof(int)*col_size);

    // OSTimeSet(0);
    for (i = 0; i < N_TASKS; i++) {                        /* Create real-time tasks (the highest priority = 1) */
        OSTaskCreate(Task, (void *)&TaskArgs[i], &TaskStk[i][TASK_STK_SIZE - 1], i + 1);                
    }

}

/*
*********************************************************************************************************
*                                                  TASKS
*********************************************************************************************************
*/

void Task (void *pdata)
{
    int start = 0;   // task start time
    int end;
    int toDelay;
    
    TASK_ARGS *tmp = (TASK_ARGS *)pdata;
    int c = tmp->compTime;
    
    OS_ENTER_CRITICAL();                                   /* Store CPU Time & Period to task's TCB    */
    OSTCBCur->compTime = c;
    OSTCBCur->period = tmp->period;
    OSTCBCur->deadline = tmp->deadline;
    OS_EXIT_CRITICAL();

    while(1)
    {
        while(OSTCBCur->compTime > 0)
        {
            // Comsume compTime and do nothing
        }
        
        OS_ENTER_CRITICAL();                               /* Disable interrupt to acquire current TCB  */
        end = OSTimeGet();
        toDelay = (OSTCBCur->period) - (end - start);
        start = start + (OSTCBCur->period);               /* Update to the next start time                */
        OSTCBCur->compTime = c;               /* Reset CPU Time */
        OSTCBCur->deadline = start + OSTCBCur->period;    /* Update deadline */

        if(toDelay < 0)
            printf("Task%d deadline\n", OSTCBCur->OSTCBPrio);
        else
            if(OSTCBCur->OSTCBPrio == 1)
                Print();
        OSTimeDly(toDelay);
        OS_EXIT_CRITICAL();

    }
}

void Print(void)
{
    int i = 0;
    if(print_pos<pos){
        for(i=print_pos; i<pos; i++){
            printf("%d\t", buf[i][0]);
            if(buf[i][1]==0)    printf("Preempt\t");
            else                printf("Complete\t");
            printf("%d\t%d\n", buf[i][2], buf[i][3]);
        }
    }
    else{
        for(i=print_pos; i<row_size; i++){
            printf("%d\t", buf[i][0]);
            if(buf[i][1]==0)    printf("Preempt\t");
            else                printf("Complete\t");
            printf("%d\t%d\n", buf[i][2], buf[i][3]);
        }
        for(i=0; i<pos; i++){
            printf("%d\t", buf[i][0]);
            if(buf[i][1]==0)    printf("Preempt\t");
            else                printf("Complete\t");
            printf("%d\t%d\n", buf[i][2], buf[i][3]);
        }
    }
    print_pos = pos;
}

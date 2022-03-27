#pragma once

// Protothread class and macros for lightweight, stackless threads in C++.
//
// This was "ported" to C++ from Adam Dunkels' protothreads C library at:
//     http://www.sics.se/~adam/pt/
//
// !!! Do NOT use 'break' in task !!!
// !!! Do NOT use 'break' in task !!!
// !!! Do NOT use 'break' in task !!!

/* E.g.

class Pt_Task
{
public:
    void PeriodicRun();
private:
    int task1Line;
    Timer task1Tmr;
    bool Task1(int * _ptLine);

    int task2Line;
    Timer task2Tmr;
    bool Task2(int * _ptLine);

};

void Pt_Task::PeriodicRun()
{
    Task1(&task1Line);
    Task2(&task2Line);
}

bool Pt_Task::Task1(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        printf("Task1 Step 0, delay 1 sec\n");
        task1Tmr.Setms(1000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
        printf("Task1 Step 1, delay 3 sec\n");
        task1Tmr.Setms(3000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
        printf("Task1 Step 2, delay 5 sec\n");
        task1Tmr.Setms(5000);
        PT_WAIT_UNTIL(task1Tmr.IsExpired());
    };
    PT_END();
};

bool Pt_Task::Task2(int *_ptLine)
{
    PT_BEGIN();
    while (true)
    {
        printf("Task2, every 2 sec\n");
        task2Tmr.Setms(2000);
        PT_WAIT_UNTIL(task2Tmr.IsExpired());
    };
    PT_END();
};
*/

// bool Pt_Task::Task1(int *_ptLine);
// params:  *_ptLine:   Task State
//          0:      Task Start
//          -1:     Task Exit, will not run unless reset *_ptLine as 0
//          other:  In a Task State
// return:  bool:   Task status
//          false:  Task finished
//          true:   Task is running
#define PT_RUNNING  true
#define PT_NOT_RUNNING  false

// Declare start of protothread (use at start of TASK(int *_ptLine) implementation).
#define PT_BEGIN() bool ptYielded = true; switch (*_ptLine) { case 0:

// Stop protothread and end it (use at end of TASK(int *_ptLine) implementation).
#define PT_END() default: ; } *_ptLine=-1 ; return false;

// Cause protothread to wait until given condition is true.
#define PT_WAIT_UNTIL(condition) \
    do { *_ptLine = __LINE__; case __LINE__: \
    if (!(condition)) return true; } while (0)

// Cause protothread to wait while given condition is true.
#define PT_WAIT_WHILE(condition) PT_WAIT_UNTIL(!(condition))

// Cause protothread to setup a entry for next call of task
#define PT_NEW_ENTRY() PT_WAIT_UNTIL(true)

// Cause protothread to wait until given child protothread completes.
#define PT_WAIT_TASK(child) PT_WAIT_UNTIL(!(child))

// Restart and spawn given child protothread and wait until it completes.
//#define PT_SPAWN(child) \
//    do { (child).Restart(); PT_WAIT_THREAD(child); } while (0)

// Restart protothread's execution at its PT_BEGIN.
#define PT_RESTART() do { *_ptLine=0; return true; } while (0)

// Exit from protothread and reset.
#define PT_ESCAPE() do { *_ptLine=0; return false; } while (0)

// Stop and exit from protothread.
#define PT_EXIT() do { *_ptLine=-1; return false; } while (0)

// Yield protothread till next call to its TASK().
#define PT_YIELD() \
    do { ptYielded = false; *_ptLine = __LINE__; case __LINE__: \
    if (!ptYielded) return true; } while (0)

// Yield protothread until given condition is true.
#define PT_YIELD_UNTIL(condition) \
    do { ptYielded = false; *_ptLine = __LINE__; case __LINE__: \
    if (!ptYielded || !(condition)) return true; } while (0)


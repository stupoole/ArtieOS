//
// Created by artypoole on 21/11/24.
//

#ifndef PROCESS_H
#define PROCESS_H

#include "types.h"
#include "CPU.h"

#ifdef __cplusplus
//https://en.wikipedia.org/wiki/Task_state_segment
struct Process
{
    Process();
    void reset();
    ~Process() = default;

    enum State_t
    {
        STATE_DEAD,
        STATE_EXITED,
        STATE_PARKED,
        STATE_READY,
    };

    // Used to scale execution duration.
    enum Priority_t
    {
        PRIORITY_LOW = 1,
        PRIORITY_NORMAL = 2,
        PRIORITY_HIGH = 4,
    };

    bool isParked() { return state == STATE_PARKED; }
    bool isDead() { return state == STATE_DEAD; }

    u32 parent_pid;
    State_t state;
    Priority_t priority;
    size_t last_executed;
    cpu_registers_t context;
    //    u32 base_vaddr;
    char name[32]; //this can be stored in an equivalent of proc?
    void* stack;
};

inline Process::Process()
{
    state = STATE_DEAD;
    parent_pid = -1;
    priority = PRIORITY_NORMAL;
    last_executed = 0;
    context = cpu_registers_t{};
    stack = NULL;
    name[0] = '\0';
}

inline void Process::reset()
{
    state = STATE_DEAD;
    parent_pid = -1;
    priority = PRIORITY_NORMAL;
    last_executed = 0;
    context = cpu_registers_t{};
    stack = NULL;
    name[0] = '\0';
}


extern "C" {
#endif


#ifdef __cplusplus
}
#endif

#endif //PROCESS_H

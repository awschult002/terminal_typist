#ifndef TIMER_T
#define TIMER_T
#include <time.h>
#include <stdbool.h>

struct timer_t
{
    clock_t start_time, set_time;
};

void set_timer(struct timer_t *tmr, unsigned long miliseconds)
{
    tmr->set_time = miliseconds;
}

void start_timer(struct timer_t *tmr)
{
    tmr->start_time = clock();
}

bool timer_elapsed(struct timer_t *tmr)
{
    clock_t now = clock();
    clock_t diff =(now - tmr->start_time);
    long long ticks = CLOCKS_PER_SEC;
    long long mticks = ticks/1000;
    long long elapsed = diff/mticks;
    if(elapsed > tmr->set_time)
        return 1;
    else
        return 0;
}


#endif

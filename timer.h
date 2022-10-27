#ifndef TIMER_T
#define TIMER_T
#include <time.h>
#include <stdbool.h>

struct timer_t
{
    double start_time, set_time;
};

void set_timer(struct timer_t *tmr, unsigned long miliseconds)
{
    tmr->set_time = miliseconds/1000.0;
}

double timespec2sec(struct timespec tp)
{
    return (double)tp.tv_sec + tp.tv_nsec/(1000.0*1000.0*1000.0);
}

void start_timer(struct timer_t *tmr)
{
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    tmr->start_time = timespec2sec(now);
}

/*
void diff_timespec(struct timespec *l, struct timespec *r, struct timespec *out)
{
    out->tv_sec = (l->tv_sec - r->tv_sec);
    out->tv_nsec = (l->tv_nsec - r->tv_nsec);
}
*/

bool timer_elapsed(struct timer_t *tmr)
{
    struct timespec now; clock_gettime(CLOCK_REALTIME, &now);
    double diff = (timespec2sec(now) - tmr->start_time);
    if(diff > tmr->set_time)
    {
        return 1;
    }

    return 0;
}


#endif

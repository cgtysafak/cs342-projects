#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include "rm.h"

#define NUMR 1        // number of resource types
#define NUMP 2        // number of threads

int AVOID = 1;
int exist[1] =  {8};  // resources existing in the system

void pr (int tid, char astr[], int m, int r[])
{
    int i;
    printf ("thread %d, %s, [", tid, astr);
    for (i=0; i<m; ++i) {
        if (i==(m-1))
            printf ("%d", r[i]);
        else
            printf ("%d,", r[i]);
    }
    printf ("]\n");
}


void setarray (int r[MAXR], int m, ...)
{
    va_list valist;
    int i;
    
    va_start (valist, m);
    for (i = 0; i < m; i++) {
        r[i] = va_arg(valist, int);
    }
    va_end(valist);
    return;
}


void *threadfunc1 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];
    
    tid = *((int*)a);
    rm_thread_started (tid);

    setarray(claim, NUMR, 8);
    rm_claim (claim);
    
    setarray(request1, NUMR, 5);
    pr (tid, "REQ", NUMR, request1);
    rm_request (request1);

    sleep(4);

    setarray(request2, NUMR, 3);
    pr (tid, "REQ", NUMR, request2);
    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended();
    pthread_exit(NULL);
}


void *threadfunc2 (void *a)
{
    int tid;
    int request1[MAXR];
    int request2[MAXR];
    int claim[MAXR];

    tid = *((int*)a);
    rm_thread_started (tid);

    setarray(claim, NUMR, 8);
    rm_claim (claim);

    setarray(request1, NUMR, 2);
    pr (tid, "REQ", NUMR, request1);
    rm_request (request1);

    sleep(2);
    
    setarray(request2, NUMR, 4);
    pr (tid, "REQ", NUMR, request2);
    rm_request (request2);

    rm_release (request1);
    rm_release (request2);

    rm_thread_ended ();
    pthread_exit(NULL);
}


int main(int argc, char **argv)
{
    int i;
    int tids[NUMP];
    pthread_t threadArray[NUMP];
    int count;
    int ret;

    if (argc != 2) {
        printf ("usage: ./app avoidflag\n");
        exit (1);
    }

    AVOID = atoi (argv[1]);
    
    if (AVOID == 1)
        rm_init (NUMP, NUMR, exist, 1);
    else
        rm_init (NUMP, NUMR, exist, 0);

    i = 0;  // we select a tid for the thread
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc1, (void *)
                    (void*)&tids[i]);
    
    i = 1;  // we select a tid for the thread
    tids[i] = i;
    pthread_create (&(threadArray[i]), NULL,
                    (void *) threadfunc2, (void *)
                    (void*)&tids[i]);

    count = 0;
    while ( count < 10) {
        sleep(1);
        rm_print_state("The current state");
        ret = rm_detection();
        if (ret > 0) {
            printf ("deadlock detected, count=%d\n", ret);
            rm_print_state("state after deadlock");
        }
        count++;
    }
    
    if (ret == 0) {
        for (i = 0; i < NUMP; ++i) {
            pthread_join (threadArray[i], NULL);
            printf ("joined\n");
        }
    }
}


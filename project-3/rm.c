#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"


// global variables

int DA;  // indicates if deadlocks will be avoided or not
int N;   // number of processes
int M;   // number of resource types
int ExistingRes[MAXR]; // Existing resources vector

//..... other definitions/variables .....
//.....
//.....

// end of global variables

int rm_thread_started(int tid)
{
    int ret = 0;
    return (ret);
}


int rm_thread_ended()
{
    int ret = 0;
    return (ret);
}


int rm_claim (int claim[])
{
    int ret = 0;
    return(ret);
}


int rm_init(int p_count, int r_count, int r_exist[],  int avoid)
{
    int i;
    int ret = 0;
    
    DA = avoid;
    N = p_count;
    M = r_count;
    // initialize (create) resources
    for (i = 0; i < M; ++i)
        ExistingRes[i] = r_exist[i];
    // resources initialized (created)
    
    //....
    // ...
    return  (ret);
}


int rm_request (int request[])
{
    int ret = 0;
    
    return(ret);
}


int rm_release (int release[])
{
    int ret = 0;

    return (ret);
}


int rm_detection()
{
    int ret = 0;
    
    return (ret);
}


void rm_print_state (char hmsg[])
{
    return;
}

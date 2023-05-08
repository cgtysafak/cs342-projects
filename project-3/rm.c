#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include "rm.h"
#include <stdbool.h>


// global variables

int DA;  // indicates if deadlocks will be avoided or not
int N;   // number of processes
int M;   // number of resource types
int ExistingRes[MAXR]; // Existing resources vector

//..... other definitions/variables .....
int AvailableRes[MAXR];

int MaxDemand[MAXP][MAXR];
int Allocation[MAXP][MAXR];
int RequestRes[MAXP][MAXR];
int Need[MAXP][MAXR];

bool is_alive[MAXP];  //  bunları başta false nasıl yapabilirim ????   ///

long map_tid[MAXP];

pthread_mutex_t lock;

pthread_cond_t cv;


//.....
//.....

// end of global variables

int rm_thread_started(int tid)
{
    int ret = 0;
    printf("self id: %d\n", pthread_self());
    map_tid[tid] = pthread_self();
    is_alive[tid] = true;
    return (ret);
}


int rm_thread_ended()
{
    int ret = 0;

    for(int i=0; i<MAXP; i++)
    {
        if(pthread_self() == map_tid[i])
        {    
            printf("ended thread is %d\n", i);
            is_alive[i] = false;

        }
    }        

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

    if( p_count < 0 || p_count > 100 || r_count < 0 || r_count > 100)
    {
        return -1;
    }    
    
    // initialize (create) resources
    for (i = 0; i < M; ++i)
    {    
        ExistingRes[i] = r_exist[i];
        AvailableRes[i] = r_exist[i];
    }
    // resources initialized (created)

    for (i = 0; i < N; ++i)
    { 
        is_alive[i] = false;
        for(int j=0; j<M; j++)  
        { 
            Allocation[i][j] = 0;
            RequestRes[i][j] = 0;
        }
    }

    //initialize lock and cv variables
    pthread_mutex_init(&lock, NULL);

    pthread_cond_init(&cv, NULL);

    for (i = 0; i < M; ++i)
        printf("Existing res %d\n", ExistingRes[i]);
    //....
    // ...
    return  (ret);
}


int rm_request (int request[])
{
    int tid;
    for(int i=0; i<N; i++)
    {
        if(pthread_self() == map_tid[i])
        {    
            tid = i;
            break;

        }
    } 

    bool is_request_valid = true;

    for(int i=0; i< M; i++)
    {
        if(request[i] > AvailableRes[i])
        {    
            is_request_valid = false;
            break;
        }

    }  

    int ret = 0;

    if(is_request_valid)
    {
        printf("thread %d request resources!! \n ", tid);
        for(int i=0; i< M; i++)
        {
            AvailableRes[i] -= request[i];
            RequestRes[tid][i] = request[i];
            Allocation[tid][i] += request[i]; 
        }    
    } 
    else
        ret = -1;   
    
    return(ret);
}

int rm_release (int release[])
{
    int ret = 0;
    int tid;

    for(int i=0; i<N; i++)
    {
        if(pthread_self() == map_tid[i])
        {    
            tid = i;
            break;

        }
    } 

    bool is_release_valid = true;

    for(int i=0; i< M; i++)
    {
        if(release[i] > RequestRes[tid][i])
        {    
            is_release_valid = false;
            break;
        }

    } 

    if(is_release_valid)
    {
        printf("thread %d release resources!! \n ", tid);
        for(int i=0; i< M; i++)
        {
            AvailableRes[i] += release[i];
            RequestRes[tid][i] -= release[i];
            //Allocation[tid][i] += request[i]; 
        }    
    } 
    else
        ret = -1;  

    return (ret);
}


int rm_detection()
{
    int ret = 0;
    int work[M];
    bool finish[N];

    for(int i=0; i<M; i++)
        work[i] = AvailableRes[i];


    for(int i=0; i<N; i++)
    {
        bool is_finished = true; 
        for(int j=0; j<M; j++)
        {
            if(RequestRes[i][j] != 0)
            {    
                finish[i] = false;
                break;
            }
        }    

    }   

    bool is_detected = true;

    while(is_detected)
    {   
        is_detected = false; //in each loop if there is no finish[i] == false and 
                            //requesti < worki exist while loop
        for(int i=0; i<N; i++)
        {
            if(finish[i] == true)
                continue;
            else
            {
                bool is_request_less_than_aval = true;
                for(int j=0; j<M; j++)
                {
                    if(RequestRes[i][j] > work[j])
                    {
                        is_request_less_than_aval = false;
                    }    

                }

                if(is_request_less_than_aval)  
                {
                    is_detected = true;
                    for(int j=0; j<M; j++)
                    {
                        work[j] += Allocation[i][j]; 

                    }

                    finish[i] = true;
                }  
            }    
        }    
    } 

    for(int i=0; i<N; i++)
    {
        if(finish[i] == false)
            ret = 0; 
    }    

    return (ret);
}


void rm_print_state (char hmsg[])
{
    printf("%s\n", hmsg);

    //exist//
    printf("Exists:\n");

    for(int i=0; i<M; i++)
    {
        printf("R%d ", i);
    }   

    printf("\n");
    for(int i=0; i<M; i++)
    {
        printf("%d  ", ExistingRes[i]);
    } 

    //AVAILABLE//
    printf("\nAvailable:\n");

    for(int i=0; i<M; i++)
    {
        printf("R%d ", i);
    }   

    printf("\n");
    for(int i=0; i<M; i++)
    {
        printf("%d  ", AvailableRes[i]);
    } 

    //ALLOCATION
    printf("\nAllocation:\n");

    for(int i=0; i<M; i++)
    {
        printf("R%d ", i);
    } 

    for(int i=0; i<N; i++)
    {
        printf("\nT%d: ", i);
        for(int j=0; j<M; j++)
        {
            printf("%d  ", Allocation[i][j]);
        }    
    } 

    //REQUEST 
    printf("\nRequest:\n");

    for(int i=0; i<M; i++)
    {
        printf("R%d ", i);
    } 

    for(int i=0; i<N; i++)
    {
        printf("\nT%d: ", i);
        for(int j=0; j<M; j++)
        {
            printf("%d  ", RequestRes[i][j]);
        }    
    }   

    //the rest of information(maxdemand and need) is printed if deadlock avoidance algorithm is used
    if(DA == 1)
    {
        //TO DO
    }    


    return;
}

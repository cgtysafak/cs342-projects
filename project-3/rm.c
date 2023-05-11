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

// NEW METHOD FOR SAFETY CHECKING ALGORITHM
bool is_safe(int request[], int tid)
{
    int work[M];
    bool finish[N];
    int tempNeed[N][M];

    for(int i=0; i<M; i++)
    {
        work[i] = AvailableRes[i] - request[i];
    } 

    for(int i=0; i<N; i++)
        finish[i] = false;

    for(int i = 0; i<N; i++)
    {
        for(int j=0; j<M; j++)
        {
            tempNeed[i][j] = Need[i][j];
        } 
    }

    for(int i=0; i<M; i++)
        tempNeed[tid][i] -= request[i];


    for(int i=0; i<N; i++)
    {
        finish[i] = true; 
        for(int j=0; j<M; j++)
        {
            if(tempNeed[i][j] > 0)
            {    
                finish[i] = false;
                break;
            }
        }

        if(finish[i] == true)
        {    
            for(int j=0; j<M; j++)
            {
                if(i == tid)
                    work[j] += request[j];    
                work[j] += Allocation[i][j]; 
            } 
        }
    } 

    bool is_process_finished = false;
    while(!is_process_finished)
    {
        is_process_finished = true;
        for(int i=0; i<N; i++)
        {
            //printf("for loop thread is: %d\n", i); 
            if(finish[i] == false)
            {
                bool is_request_less_than_aval = true;
                for(int j=0; j<M; j++)
                {
                    if(tempNeed[i][j] > work[j])
                    {   
                        is_request_less_than_aval = false; 
                        break;
                    }
                } 

                if(is_request_less_than_aval)
                {
                    is_process_finished = false;
                    for(int k=0; k<M; k++)
                    {
                        if(i == tid)
                            work[k] += request[k];
                        work[k] += Allocation[i][k];
                    }
                    /* printf("finished thread is: %d\n", i); 
                    for(int l = 0; l<M; l++)
                    {
                        printf("%d  ", work[l]);
                    }  
                    printf("\n");   */
                    finish[i] = true;
                }
            }
        }
    } 

    for(int i=0; i<N; i++)
    {    
        if(finish[i] == false)
        {
            printf("this request is not safe for %d, i = %d\n", tid, i);
            //rm_print_state("nom");
            return false;
        }  
    }  

    printf("this request is safe for thread %d\n", tid);    
    return true;
}

int rm_thread_started(int tid)
{
    int ret = 0;
    //printf("self id: %ld\n", pthread_self());
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
    if(DA == 0)
        return (ret);

    int tid;
    for(int i=0; i<N; i++)
    {
        if(pthread_self() == map_tid[i])
        {    
            tid = i;
            break;

        }
    } 

    for(int i = 0; i<M; i++)
    {
        if(claim[i] > ExistingRes[i])
            return -1;

        MaxDemand[tid][i] = claim[i];
        Need[tid][i] = claim[i];
    }  

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
        if(request[i] > ExistingRes[i])
        {    
            is_request_valid = false;
            break;
        }
    } 

    if(!is_request_valid) 
        return -1;

    if(DA == 0)
    {
        pthread_mutex_lock(&lock);
        for(int i=0; i< M; i++)
            RequestRes[tid][i] += request[i]; 

        while(true)
        {    
            bool is_request_available = true;

            for(int i=0; i< M; i++)
            {
                if(request[i] > AvailableRes[i])
                {    
                    is_request_available = false;
                    break;
                }

            } 
            if(is_request_available)
            {
                //pthread_mutex_lock(&lock);     //lockların yerini değiştirdim cünkü update olduğunda sıkıntı olmasın
                printf("thread %d request resources!! \n ", tid);
                for(int i=0; i< M; i++)
                {
                    AvailableRes[i] -= request[i];
                    RequestRes[tid][i] -= request[i];
                    Allocation[tid][i] += request[i]; 
                } 
                pthread_mutex_unlock(&lock);
                return 0;   
            } 
            else
            {   
                //return -1; //burayı sonradan değiştir
                pthread_mutex_unlock(&lock);
                pthread_cond_wait(&cv, &lock);  
            }
            
        }
    }
    
    else if(DA== 1)
    {
        //TO DO
        pthread_mutex_lock(&lock); 
        for(int i=0; i< M; i++)
            RequestRes[tid][i] += request[i];  

        while(true)
        {    
            bool is_request_available = true;

            for(int i=0; i< M; i++)
            {
                if(request[i] > AvailableRes[i])
                {    
                    is_request_available = false;
                    break;
                }

            } 
            if(is_request_available)
            {
                if(is_safe(request, tid))
                {
                    printf("thread %d request resources!! \n ", tid);
                    for(int i=0; i< M; i++)
                    {
                        AvailableRes[i] -= request[i];
                        Allocation[tid][i] += request[i]; 
                        Need[tid][i] -= request[i];
                        RequestRes[tid][i] -= request[i];
                    } 
                    pthread_mutex_unlock(&lock);
                    return 0;
                }        
            }

            else
                printf("resources are not available for %d\n", tid);

            pthread_mutex_unlock(&lock);
            pthread_cond_wait(&cv, &lock);      
        }        
        return 0;
    }  
    return -1;  
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
        if(release[i] > Allocation[tid][i])
        {    
            is_release_valid = false;
            break;
        }

    } 

    if(is_release_valid)
    {
        pthread_mutex_lock(&lock);
        printf("thread %d release resources!! \n ", tid);
        for(int i=0; i< M; i++)
        {
            AvailableRes[i] += release[i];
            //RequestRes[tid][i] -= release[i];
            Allocation[tid][i] -= release[i]; 
        }
        pthread_mutex_unlock(&lock); 
        pthread_cond_signal(&cv);   
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
        finish[i] = true; 
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
            if(finish[i] == false)
            {
                bool is_request_less_than_aval = true;
                for(int j=0; j<M; j++)
                {
                    if(RequestRes[i][j] > work[j])
                    {
                        is_request_less_than_aval = false;
                        break;
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

    int number_of_deadlocks = 0;
    for(int i=0; i<N; i++)
    {
        if(finish[i] == false)
            number_of_deadlocks+=1; 
    }    

    return (number_of_deadlocks);
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
        printf("   R%d ", i);
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
        printf("   R%d ", i);
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
        //max demand 
            //REQUEST 
        printf("\nMAX DEMAND:\n");

        for(int i=0; i<M; i++)
        {
            printf("   R%d ", i);
        } 

        for(int i=0; i<N; i++)
        {
            printf("\nT%d: ", i);
            for(int j=0; j<M; j++)
            {
                printf("%d  ", MaxDemand[i][j]);
            }    
        }

        //NEED 
        printf("\nNeed:\n");

        for(int i=0; i<M; i++)
        {
            printf("   R%d ", i);
        } 

        for(int i=0; i<N; i++)
        {
            printf("\nT%d: ", i);
            for(int j=0; j<M; j++)
            {
                printf("%d  ", Need[i][j]);
            }    
        } 

        printf("\n------------------------------------------\n");


    }    

    return;
}




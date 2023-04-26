#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t lock[10];
pthread_mutex_t main_lock;

typedef struct BurstNode
{
    int pid;
    int burst_length;
    int arrival_time;
    int remaining_time;
    int finish_time;
    int turnaround_time;
    int cpu_id;
    struct BurstNode* next;
} BurstNode;

typedef struct thread_struct
{
    int cpu_id;
    int current_time;
    struct BurstNode* burst;
    char* alg;
    char* sap;
    char* qs;

} thread_struct;


void insert(struct BurstNode* previous, int pid, int length, int arrival_time)
{
    struct BurstNode* new_node
        = (struct BurstNode*)malloc(sizeof(struct BurstNode));
        
    new_node->pid = pid;
    new_node->burst_length = length;
    new_node->arrival_time = arrival_time;
    new_node->remaining_time = length;
    new_node->finish_time = 0;
    new_node->turnaround_time = 0;
    new_node->cpu_id = 0;
    new_node->next = NULL;
    
    new_node->next = previous->next;
    
    previous->next= new_node;
}

void push(struct BurstNode** head, int pid, int length, int arrival_time)
{
    struct BurstNode* new_node
        = (struct BurstNode*)malloc(sizeof(struct BurstNode));
 
    new_node->pid = pid;
    new_node->burst_length = length;
    new_node->arrival_time = arrival_time;
    new_node->remaining_time = length;
    new_node->finish_time = 0;
    new_node->turnaround_time = 0;
    new_node->cpu_id = 0;
    new_node->next = NULL;
 
    new_node->next = (*head);
  
    *head = new_node; //change the address of head pointer
}

//print each element in linked list
void printList(struct BurstNode* head)
{
    struct BurstNode* temp = head;
    while(temp != NULL)
    {
        printf("pid: %d -->", temp->pid);
        printf("length %d\n", temp->burst_length);
        temp = temp->next;
    }
}

void printBursts( struct BurstNode* head)
{
    struct BurstNode* temp = head;
    while(temp != NULL && temp->pid != -1)
    {
        printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\n");
        printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\n", temp->pid, temp->cpu_id, temp->burst_length, temp->arrival_time,
                                                    temp->finish_time, (temp->turnaround_time-temp->burst_length), 
                                                    temp->turnaround_time);
        temp = temp->next;
    }
}

void *cpu_process(void *args)
{
    thread_struct *arg = (thread_struct*) args;

    int cpu_id = arg->cpu_id;
    BurstNode* temp = arg->burst;
    char* alg = arg->alg;
    char *sap = arg->sap;
    char *qs = arg->qs;    

    //int time = arg->current_time + arg->elapsed_ptime;
    printf("Processor: %d is running \n", cpu_id);

    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    long start_ms = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;

    if(strcmp(sap, "S") == 0)
    {
        printf("S");
        //FIRST COME FIRST SERVED ALGORITHM
        if(strcmp(alg, "FCFS") == 0)
        {
            bool is_running = false;
            int length;
            while (temp != NULL && temp->pid != -1)
            {
                gettimeofday(&current_time, NULL);
                long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                long elapsed_ms = current_ms - start_ms;
                
                if(temp->arrival_time > (int)elapsed_ms)
                {
                    usleep((temp->arrival_time - (int)elapsed_ms)*1000);
                    continue;
                }    

                pthread_mutex_lock(&(main_lock)); 
                if(temp->remaining_time != 0) 
                {   
                    current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                    elapsed_ms = current_ms - start_ms; 
                    temp->remaining_time = 0;
                    temp->cpu_id = cpu_id;
                    length = temp->burst_length;
                    temp->finish_time = (int)elapsed_ms + length;
                    is_running = true;
                }

                pthread_mutex_unlock(&(main_lock));


                if(is_running)
                {   
                    printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\n");
                    printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\n", temp->pid, cpu_id, temp->burst_length, temp->arrival_time,
                                                    temp->finish_time, (temp->turnaround_time-temp->burst_length), 
                                                    temp->turnaround_time); 
                    usleep(length*1000);
                }    

                is_running = false;

                //join_thread = pthread_join(threads[i], NULL);

  
                temp = temp->next;

            }
            pthread_exit(NULL);
        }

        else if(strcmp(alg, "SJF") == 0)
        {
            BurstNode* min_node;
            BurstNode* temp = arg->burst;
            int min_length;
            bool dummy_item = false;
            int length;
            
            while(!dummy_item)
            {
                temp = arg->burst;
                dummy_item = true;
                while( temp != NULL)
                {
                    if(temp->remaining_time != 0)
                        dummy_item = false; 
                    temp = temp->next;     

                } 
                if(dummy_item) 
                    break;

                gettimeofday(&current_time, NULL);
                long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                long elapsed_ms = current_ms - start_ms;
                temp = arg->burst;
                min_node = NULL;
                min_length = 999999;
                while(temp != NULL && temp->pid != -1) 
                {
                    if(temp->burst_length < min_length && temp->remaining_time!=0 && temp->pid!=-1
                                    && temp->arrival_time <= (int)elapsed_ms)
                    {    
                        min_length = temp->burst_length;
                        min_node = temp;
                    }

                    if(temp->arrival_time > (int)elapsed_ms)
                        break;

                    temp = temp->next;
                }
    
                
                gettimeofday(&current_time, NULL);
                current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                elapsed_ms = current_ms - start_ms;

                if(min_node== NULL)
                {
                    current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                    elapsed_ms = current_ms - start_ms;
                    usleep((temp->arrival_time - (int)elapsed_ms)*1000);
                    continue;         
                } 
 

                printf("Pid:%d", min_node->pid );
                
                pthread_mutex_lock(&(main_lock)); 
                if(min_node->remaining_time != 0) 
                {   
                    long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                    long elapsed_ms = current_ms - start_ms; 
                    min_node->remaining_time = 0;
                    min_node->cpu_id = cpu_id;
                    length = min_node->burst_length;
                    min_node->finish_time = (int)elapsed_ms + length;
                }

                pthread_mutex_unlock(&(main_lock));

                printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\n");
                printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\n", min_node->pid, cpu_id, min_node->burst_length, min_node->arrival_time,
                                                    min_node->finish_time, (min_node->turnaround_time-min_node->burst_length), 
                                                    min_node->turnaround_time); 

                usleep(length*1000);
                continue;

            } 
            pthread_exit(NULL);
        }
    
    }
    pthread_exit(NULL);
    return NULL;
}  


int main(int argc, char *argv[])
{
    int n = 2;                  // default value for -n
    char* sap = "S";            // default value for -a
    char* qs = "RM";            // default value for -a (multi-queue)
    char* alg = "SJF";           // default value for -s
    int q = 20;                 // default value for -s (RR quantum)
    char* infile = "in.txt";    // default value for -i
    int outmode = 1;            // default value for -m
    char* outfile = NULL;       // default value for -o
    // int t = 0, t1 = 0, t2 = 0, l = 0, l1 = 0, l2 = 0, pc = 0;    // default value for -r

    int opt;
    // Receive arguments...
    while ((opt = getopt(argc, argv, "n:a:s:i:m:o:r:")) != -1)
    {
        switch (opt)
        {
            case 'n':
                n = atoi(optarg);
                break;
            case 'a':
                sap = optarg;
                
                if (strcmp(sap, "S") == 0)
                    qs = "NA";
                else if (strcmp(sap, "M") == 0)
                    qs = argv[optind++];

                break;
            case 's':
                alg = optarg;
                if (strcmp(alg, "RR") == 0)
                    q = atoi(argv[optind++]);
                else
                    q = 0;

                break;
            case 'i':
                infile = optarg;
                break;
            case 'm':
                outmode = atoi(optarg);
            case 'o':
                outfile = optarg;
                outmode = 0;

                break;
            default:
                fprintf(stderr,
                "Usage: %s [-n N] [-a SAP QS] [-s ALG Q] [-i INFILE] [-m OUTMODE] [-o OUTFILE] [-r T T1 T2 L L1 L2 PC]\n",
                argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    if (n<2 && strcmp(alg, "RR") == 0)
    {
        fprintf(stderr, "Error: Number of processors must be greater than 2 to use multi-queue approach.\n");
        exit(EXIT_FAILURE);
    }
    else if (outmode < 0 || outmode > 3)
    {
        fprintf(stderr, "Error: -o (OUTMODE) has an invalid value.\n");
        exit(EXIT_FAILURE);
    }

    printf("n = %d\n", n);
    printf("sap = %s\n", sap);
    printf("qs = %s\n", qs);
    printf("alg = %s\n", alg);
    printf("q = %d\n", q);
    printf("infile = %s\n", infile);
    printf("outmode = %d\n", outmode);
    if (outfile == NULL)
        printf("outfile = (null)\n");
    else
        printf("outfile = %s\n", outfile);

    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    long start_ms = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;

    //THREAD CREATION
    pthread_t threads[n];
    int cpu_ids[n]; 
    int create_thread;
    int join_thread;
    thread_struct args[n];

    for(int i=0; i<n; i++)
        pthread_mutex_init(&lock[i], NULL);

    pthread_mutex_init(&main_lock, NULL);

    //BURST CREATION
    BurstNode* head = NULL;
    BurstNode* temp = NULL;
    int burst_length, arrival_time = 0;

    int pid = 1;
    char *fileName = infile;
    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;
    char *token = NULL;
    fp = fopen(fileName, "r");
    if (fp == NULL)
        exit(EXIT_FAILURE);

    // Reading the file...
    while ((read = getline(&line, &len, fp)) != -1)
    {
        char lie[len];

        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", line);

        strcpy(lie, line);
        token = strtok(lie, " ");

        if (token != NULL && strcmp(token, "PL") == 0)
        {
            //printf("First word: %s\n", token);
            token = strtok(NULL, " ");
            //printf("Length: %s\n", token);
            burst_length = atoi(token);
            if(head == NULL)
            {
                push(&head, pid, burst_length, arrival_time);
                temp = head;
                pid++;
            }   
            else
            {
                insert(temp, pid, burst_length, arrival_time);
                temp = temp->next;
                pid++;
            }
        }

        if (token != NULL && strcmp(token, "IAT") == 0)
        {
            //printf("First word: %s\n", token);
            token = strtok(NULL, " ");
            //printf("Arrival time: %s\n", token);
            arrival_time += atoi(token);
        }
    }
    insert(temp, -1, 0, 0);    
    fclose(fp);
    if (line)
        free(line);

    printList(head);
    printBursts(head);

    for(int i = 0; i < n; i++)
    {
        cpu_ids[i] = i;
        BurstNode* initial = NULL;
        int elapsed_ptime = 0;
        args[i].cpu_id = i;
        args[i].current_time = 0;
        args[i].burst = head;
        args[i].alg = alg;
        args[i].sap = sap;
        args[i].qs = qs;

        //void *args[3] = {&cpu_ids[i], &elapsed_ptime, &initial};
        //void *args[3] = {&cpu_ids[i], 0, NULL};
        //create_thread = pthread_create(&threads[i], NULL, cpu_process, (void*)args);
        create_thread = pthread_create(&threads[i], NULL, cpu_process, &args[i]);   
        if(create_thread) 
        {
            printf("Processor: %d", cpu_ids[i]);
            printf("Error: return code from pthred_create() is %d\n", create_thread);
            return -1;
        }
        printf("for loop: %d\n", i);
    }   


    for( int i=0; i<n; i++)
    {

        join_thread = pthread_join(threads[i], NULL);
        if(join_thread)
        {
            printf("Processor: %d", cpu_ids[i]);
            printf("Error: return code from pthread_join() is %d\n", join_thread);
            return -1;
        }
        else
        {
            printf("Processor: %d is succesfully finished\n", cpu_ids[i]);
        }  
    } 

    //SINGLE QUEUE APPROACH
    if(strcmp(sap, "S") == 0)
    {
        printf("S");
        //FIRST COME FIRST SERVED ALGORITHM
        if(strcmp(alg, "FCF") == 0)
        {
            BurstNode* temp = head;
            int available_processor_no = -1;
            bool available_processor = false;
            while (temp != NULL)
            {
                available_processor = false;
                while (!available_processor)
                {  
                    thread_struct* arg = (thread_struct*) malloc(n * sizeof(thread_struct));  
                    for( int i = 0; i<n; i++)
                    {
                        if( !pthread_mutex_trylock(&lock[i]))
                        {
                            printf("buraya girdi%d", i);
                            available_processor_no = i;
                            available_processor = true;
                            args[i].cpu_id = cpu_ids[i];
                            //args[i].elapsed_ptime = 0;  //temp->burst_length;
                            args[i].current_time = 0;
                            args[i].burst = NULL;
                            create_thread = pthread_create(&threads[i], NULL, cpu_process, &args[i]);   
                            if(create_thread) 
                            {
                                printf("Processor: %d", cpu_ids[i]);
                                printf("Error: return code from pthred_create() is %d\n", create_thread);
                                return -1;
                            }
                            join_thread = pthread_join(threads[i], NULL);
                            if(join_thread)
                            {
                                printf("Processor: %d", cpu_ids[i]);
                                printf("Error: return code from pthread_join() is %d\n", join_thread);
                                return -1;
                            }
                            else
                            {
                                printf("Processor: %d is succesfully finished\n", cpu_ids[i]);
                            } 
                            free(arg);

                            printf("burası da\n");   

                        }  

                        if(available_processor)
                            break;  
                    } 

                    /*
                    int is_available = 0;
                    pthread_mutex_lock(&main_lock);

                    printf("%d", is_available);

                    while(!is_available)
                    {    
                        for( int i = 0; i < n; i++)
                        {
                            if( !pthread_mutex_trylock(&lock[i]))
                            {    
                                is_available = 1;
                                break;
                            }

                        } 
                    }  

                    usleep(0);

                    pthread_mutex_unlock(&main_lock);  */

                    /*
                    printf("off");
                    if (!available_processor)
                    {  
                        printf("join0");  
                        for( int i = 0; i<n; i++)
                        {    
                            //usleep(1000);
                            printf("join1");
                            join_thread = pthread_join(threads[i], NULL);
                            printf("join");
                            if(join_thread)
                            {
                                printf("Processor: %d", cpu_ids[i]);
                                printf("Error: return code from pthread_join() is %d\n", join_thread);
                                return -1;
                            }
                            else
                            {
                                printf("Processor: %d is succesfully finished\n", cpu_ids[i]);
                            } 
                        }
                    }  */
                }   
                
                temp = temp->next;

            }
        }

        else if(strcmp(alg, "SJF") == 0)
        {
            BurstNode* min_node;
            BurstNode* temp;
            int min_length;
            bool dummy_item = false;
            while(!dummy_item)
            {
                //WE CAN PUT THAT CODE TO INSIDE A METHOD, OTHERWISE ALL CODES CAN BE CONFUSING
                min_node = head;
                temp = head;
                min_length = head->burst_length;
                while(temp != NULL && temp->pid != -1) 
                {
                    if(temp->burst_length < min_length ) //&& temp->arrival_time <= current_time
                    {
                        min_length = temp->burst_length;
                        min_node = temp;
                    } 
                    temp = temp->next;
                }
                dummy_item = true;
                bool available_processor = false;
                //check processors respectively, until find an available processor
                //if not, wait until one of them will be available
                while(!available_processor)
                {
                    available_processor = true;
                    for(int i = 0; i < n; i++)
                    {
                        // TODO nasıl yapılacak bu kısım bilmiyorum 
                    }
                }
            }
        }
        else if(strcmp(alg, "RR") == 0)
        {
            BurstNode* temp;
            bool dummy_item = false;
            while(dummy_item)
            {
                //WE CAN PUT THAT CODE TO INSIDE A METHOD, OTHERWISE ALL CODES CAN BE CONFUSING
                temp = head;
                
                dummy_item = true;

                while( temp != NULL) 
                {
                    bool available_processor = false;
                    //check processors respectively, until find an available processor
                    //if not, wait until one of will be available
                    while(!available_processor)
                    {
                        available_processor = true;
                        for(int i = 0; i < n; i++)
                        {
                            // TO DO nasıl yapılacak bu kısım bilmiyorum
                            // if it is available bunun kodu yazılacak
                            if( available_processor)
                            {
                                if(temp->remaining_time > 20)
                                    temp->remaining_time -= 20;
                                else
                                {
                                    int elapsed_time = temp->remaining_time;
                                    temp->remaining_time = 0;
                                }

                                break;
                            }
                        }
                    }
                    temp = temp->next;
                    while(temp->remaining_time == 0 && temp != NULL)
                        temp = temp->next;
                }
            }
        }
        else
        {
            // Error
        }
    }
    else if(strcmp(sap, "M") == 0)
    {
        //TODO
    }

    //gettimeofday(&current_time, NULL);
    //long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
    //long elapsed_ms = current_ms - start_ms;
    //printf("Elapsed time: %ld ms\n", elapsed_ms);

    return 0;
}

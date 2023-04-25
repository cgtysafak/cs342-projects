#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>

pthread_mutex_t lock[20];

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
    int elapsed_ptime;
    int current_time;
    struct BurstNode* burst;
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
        printf("%d\t %d\t %d\t %d\t %d\t %d\t %d\n", temp->pid, temp->cpu_id, temp->burst_length, temp->arrival_time,
                                                    temp->finish_time, (temp->turnaround_time-temp->burst_length), 
                                                    temp->turnaround_time);
        temp = temp->next;
    }
}


void *cpu_process(void *args)
{
    thread_struct *arg = (thread_struct*) args;
    pthread_mutex_lock(&(lock[arg->cpu_id]));

    int time = arg->current_time + arg->elapsed_ptime;
    printf("Processor: %d is running \n", arg->cpu_id);
    
    //while(time > arg->current_time);
    
    pthread_mutex_unlock(&(lock[arg->cpu_id]));
    pthread_exit(NULL);
    return NULL;
}  


int main(int argc, char *argv[])
{
    int n = 2;                  // default value for -n
    char* sap = "M";            // default value for -a
    char* qs = "RM";            // default value for -a (multi-queue)
    char* alg = "RR";           // default value for -s
    int q = 20;                 // default value for -s (RR quantum)
    char* infile = "in.txt";    // default value for -i
    int outmode = 1;            // default value for -m
    char* outfile = NULL;       // default value for -o
    // int t = 0, t1 = 0, t2 = 0, l = 0, l1 = 0, l2 = 0, pc = 0;    // default value for -r

    int opt;

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

    // Do some work here...

    
    //THREAD CREATION
    pthread_t threads[n];
    int cpu_ids[n]; 
    int create_thread;
    int join_thread;
    thread_struct args[n];

    for(int i=0; i<n; i++)
    {
        pthread_mutex_init(&lock[i], NULL);
    }    

    for(int i = 0; i < n; i++)
    {
        cpu_ids[i] = i;
        BurstNode* initial = NULL;
        int elapsed_ptime = 0;
        args[i].cpu_id = cpu_ids[i];
        args[i].elapsed_ptime = 0;
        args[i].current_time = 0;
        args[i].burst = NULL;

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
    fclose(fp);
    if (line)
        free(line);

    printList(head);
    printBursts(head);

    //SINGLE QUEUE APPROACH
    //FCFS algorithm 
  

    //SJF algorithm picking shortest job from linked list
    if(alg == "SJF") 
    { 
        BurstNode* min_node;
        BurstNode* tmp;
        int min_length;
        bool dummy_item = false;
        while(!dummy_item)
        {
            //WE CAN PUT THAT CODE TO INSIDE A METHOD, OTHERWISE ALL CODES CAN BE CONFUSING
            min_node = head;
            tmp = head;
            min_length = head->burst_length;
            while(tmp != NULL)
            {    
                if(tmp->burst_length < min_length)
                {
                    min_length = tmp->burst_length;
                    min_node = tmp;
                } 
                tmp = tmp->next;
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

    //ROUND ROBIN ALGORITHM
    if( alg == "RR")
    {
        BurstNode* tmp;
        bool dummy_item = false;
        while(dummy_item)
        {
            //WE CAN PUT THAT CODE TO INSIDE A METHOD, OTHERWISE ALL CODES CAN BE CONFUSING
            tmp = head;
            
            dummy_item = true;

            while( tmp != NULL) 
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
                        //if it is available bunun kodu yazılacak
                        if( available_processor)
                        {
                            if(tmp->remaining_time > 20)
                            {
                                tmp->remaining_time -= 20;
                            }

                            else
                            {
                                int elapsed_time = tmp->remaining_time;
                                tmp->remaining_time = 0;
                            }  
                            break;  

                        }    


                    }  
                }
                tmp = tmp->next;
                while(tmp->remaining_time == 0 && tmp != NULL)
                    tmp = tmp->next;
            }     

        } 
    }   

    gettimeofday(&current_time, NULL);
    long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
    long elapsed_ms = current_ms - start_ms;
    printf("Elapsed time: %ld ms\n", elapsed_ms);

    return 0;
}

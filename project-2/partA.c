#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>

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
    int q;
    int outmode;
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
    if( head == NULL)
        return;    
    int sum = 0;
    int counter = 0;
    struct BurstNode* temp = head;
    while(temp != NULL && temp->pid != -1)
    {
        sum += temp->turnaround_time;
        counter += 1;
        printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\n");
        printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\n", temp->pid, temp->cpu_id, temp->burst_length, temp->arrival_time,
                                                    temp->finish_time, (temp->turnaround_time - temp->burst_length), 
                                                    temp->turnaround_time);
        temp = temp->next;
    }
    int average_turnaround_time = sum/counter;
    printf("average turnaround time: %dms", average_turnaround_time);

}

int get_total_length(struct BurstNode* head)
{
    struct BurstNode* temp = head;
    int total_length = 0;
    while(temp != NULL && temp->pid != -1)
    {
        total_length += temp->burst_length;
        temp = temp->next;
    }
    return total_length;
}

int get_min_length(BurstNode* head[], int n)
{
    int min_length = get_total_length(head[0]);
    int min_index = 0;
    for(int i=1; i<n; i++)
    {
        if(get_total_length(head[i]) < min_length)
        {
            min_length = get_total_length(head[i]);
            min_index = i;
        }    

    }  
    return min_index;  
}

//This method is used for both length and arrival time generation
int random_generator(int T, int T1, int T2)
{
    double lambda = 1/ T;
    int x = T;
    while(true)
    {
        double u = (double) rand() / RAND_MAX;
        x = (int) (-1 * log10(1 - u) * T);
        if(x > T1 || x < T2)
            break;
    } 

    return x;
}

int generate_interarrival_time(int T, int T1, int T2) {
    double l = 1.0 / T;
    int x;
    do {
        double u = (double) rand() / RAND_MAX;
        x = (int) (-1 * log(1 - u) / l);
    } while (x < T1 || x > T2);
    return x;
}

void *cpu_process(void *args)
{
    thread_struct *arg = (thread_struct*) args;

    int cpu_id = arg->cpu_id;
    BurstNode* temp = arg->burst;
    char* alg = arg->alg;
    char *sap = arg->sap;
    char *qs = arg->qs; 
    int quantum = arg->q;
    int outmode = arg->outmode;

    //int time = arg->current_time + arg->elapsed_ptime;
    printf("Processor: %d is running \n", cpu_id);

    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    long start_ms = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;

    if(strcmp(sap, "S") == 0  || strcmp(sap, "M") == 0 )
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

                int waiting_time = 0;
                pthread_mutex_lock(&(main_lock)); 
                if(temp->remaining_time != 0) 
                {   
                    current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                    elapsed_ms = current_ms - start_ms; 
                    temp->remaining_time = 0;
                    temp->cpu_id = cpu_id;
                    length = temp->burst_length;
                    temp->finish_time = (int)elapsed_ms + length;
                    temp->turnaround_time = temp->finish_time - temp->arrival_time;
                    int waiting_time = temp->turnaround_time - temp->burst_length; 
                    is_running = true;
                }

                pthread_mutex_unlock(&(main_lock));

                if(is_running)
                {
                    printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\n");
                    printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\n", temp->pid, cpu_id, temp->burst_length, temp->arrival_time,
                                                    temp->finish_time, waiting_time, 
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
 

                //printf("Pid:%d", min_node->pid );
                
                int waiting_time = 0;
                pthread_mutex_lock(&(main_lock)); 
                if(min_node->remaining_time != 0) 
                {   
                    long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                    long elapsed_ms = current_ms - start_ms; 
                    min_node->remaining_time = 0;
                    min_node->cpu_id = cpu_id;
                    length = min_node->burst_length;
                    min_node->finish_time = (int)elapsed_ms + length;
                    min_node->turnaround_time = min_node->finish_time - min_node->arrival_time;
                    int waiting_time = min_node->turnaround_time - min_node->burst_length; 
                }

                pthread_mutex_unlock(&(main_lock));

                printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\n");
                printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\n", min_node->pid, cpu_id, min_node->burst_length, min_node->arrival_time,
                                                    min_node->finish_time, waiting_time, 
                                                    min_node->turnaround_time); 

                usleep(length*1000);
                continue;

            } 
            pthread_exit(NULL);
        }
        else if(strcmp(alg, "RR") == 0)
        {
            bool is_running = false;
            BurstNode* temp = arg->burst;
            bool dummy_item = false;
            int length;

            while (!dummy_item)
            {
                // printf("\n\n---- outermost while loop -----\n\n");

                temp = arg->burst;
                dummy_item = true;

                while (temp != NULL)
                {
                    if (temp->remaining_time != 0)
                        dummy_item = false;
                    temp = temp->next;
                }
                if (dummy_item)
                    break;

                gettimeofday(&current_time, NULL);
                long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                long elapsed_ms = current_ms - start_ms;

                temp = arg->burst;
                bool stop = false;
                while ( (temp != NULL && temp->pid != -1) && !stop)
                {
                    gettimeofday(&current_time, NULL);
                    long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                    long elapsed_ms = current_ms - start_ms;

                    // printf("\n\n---- innermost while loop -----\n\n");

                    if (temp->arrival_time > (int) elapsed_ms)
                    {
                        usleep((temp->arrival_time - (int)elapsed_ms) * 1000);
                        // usleep(quantum * 1000);

                        stop = true;
                        continue;
                    }

                    pthread_mutex_lock(&(main_lock));
                    if (temp->remaining_time > 0)
                    {
                        if (temp->remaining_time > quantum)
                        {
                            temp->remaining_time -= quantum;

                            current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                            elapsed_ms = current_ms - start_ms;

                            temp->cpu_id = cpu_id;
                            length = quantum;

                            is_running = true;
                            
                            // start_time = (int)elapsed_ms;
                        }
                        else
                        {
                            gettimeofday(&current_time, NULL);
                            long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
                            long elapsed_ms = current_ms - start_ms;
                            length = temp->remaining_time;
                            temp->remaining_time = 0;
                            
                            temp->cpu_id = cpu_id;

                            is_running = true; // ???

                            // if (start_time == 0)
                            //  start_time = (int)elapsed_ms;
                            temp->finish_time = (int)elapsed_ms + length;
                            temp->turnaround_time = temp->finish_time - temp->arrival_time;
                        }
                    }

                    if(is_running)
                    {
                        if(outmode == 2)
                        {    
                        printf("pid\t cpu\t burstlen\t arv\t finish\t waitingtime\t turnaround\t remaining_time\n");
                        printf("%d\t %d\t %d\t\t %d\t %d\t %d\t\t %d\t\t %d\n", temp->pid, cpu_id, temp->burst_length, temp->arrival_time,
                                                        temp->finish_time, (temp->turnaround_time-temp->burst_length), 
                                                        temp->turnaround_time, temp->remaining_time); 
                        }
    
                        usleep(length*1000);
                    }

                    is_running = false;
                    pthread_mutex_unlock(&(main_lock));

                    temp = temp->next;
                }
            }

            pthread_exit(NULL);
        }
        else
        {
            printf("---- buraya girmemeliydi -----");
        }
    
    }
    pthread_exit(NULL);
    return NULL;
}  


int main(int argc, char *argv[])
{
    int n = 2;                  // default value for -n
    char* sap = "M"; //S            // default value for -a
    char* qs = "RM";            // default value for -a (multi-queue)
    char* alg = "RR";           // default value for -s
    int q = 20;                 // default value for -s (RR quantum)
    char* infile = "in.txt";    // default value for -i
    int outmode = 2; //1        // default value for -m
    char* outfile = NULL;       // default value for -o
    int t = 200, t1 = 10, t2 = 1000, l = 100, l1 = 10, l2 = 500, pc = 10;    // default value for -r

    int opt;
    bool is_random = true; //false
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
            case 'r':
                is_random = true;
                    

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

    //BURST CREATION WITH INFILE
    BurstNode* head = NULL;
    BurstNode* temp = NULL;
    int burst_length, arrival_time = 0;
    if(strcmp(sap, "S") == 0 && !is_random)
    {    
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
    } 

    else if(strcmp(sap, "S") == 0 && is_random)
    {
        int pid = 1;
        arrival_time = 0;
        int interarrival_time = 0;
        for(int i = 0; i < pc; i++)
        {
            interarrival_time = random_generator(l, l1, l2);
            if(i == 0)
                interarrival_time = 0;

            arrival_time += interarrival_time;
            burst_length = random_generator(t, t1, t2);

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
        insert(temp, -1, 0, 0);   
        printList(head);
        printBursts(head);
    }    

    //MULTI QUEUE CREATION WITH INFILE
    BurstNode* heads[n];
    BurstNode* tmp[n];  
    for(int i=0; i<n; i++)
    {
        heads[i] = NULL;
        tmp[i] = NULL;
    }  

    if(strcmp(sap, "M") == 0 && !is_random)
    {    
        int burst_length = 0;
        int arrival_time = 0;

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

        if(strcmp(qs, "RM") == 0)
        {    
            int i = 0;
            while ((read = getline(&line, &len, fp)) != -1)
            {
                char lie[len];

                strcpy(lie, line);
                token = strtok(lie, " ");

                if(i >= n)
                    i = i%n;

                if (token != NULL && strcmp(token, "PL") == 0)
                {
                    token = strtok(NULL, " ");
                    burst_length = atoi(token);
                    if(heads[i] == NULL)
                    {
                        push(&heads[i], pid, burst_length, arrival_time);
                        tmp[i] = heads[i];
                        pid++;
                    }   
                    else
                    {
                        insert(tmp[i], pid, burst_length, arrival_time);
                        tmp[i] = tmp[i]->next;
                        pid++;
                    }
                    i++;
                }

                if (token != NULL && strcmp(token, "IAT") == 0)
                {
                    printf("First word: %s\n", token);
                    token = strtok(NULL, " ");
                    printf("Arrival time: %s\n", token);
                    arrival_time += atoi(token);
                }
            }
        }

        if(strcmp(qs, "LM") == 0)
        {
            while ((read = getline(&line, &len, fp)) != -1)
            {
                char lie[len];

                strcpy(lie, line);
                token = strtok(lie, " ");

                if (token != NULL && strcmp(token, "PL") == 0)
                {
                    //printf("First word: %s\n", token);
                    token = strtok(NULL, " ");
                    //printf("Length: %s\n", token);
                    bool is_inserted = false;
                    burst_length = atoi(token);
                    for(int i=0; i<n; i++)
                    {    
                        if(heads[i] == NULL)
                        {
                            push(&heads[i], pid, burst_length, arrival_time);
                            tmp[i] = heads[i];
                            is_inserted = true;
                            pid++;
                            break;
                        }

                    }   
                    if(!is_inserted)
                    {
                        int i = get_min_length(heads, n);    
                        insert(tmp[i], pid, burst_length, arrival_time);
                        tmp[i] = tmp[i]->next;
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
        }   
    }

    if(strcmp(sap, "M") == 0 && is_random)
    {
        int pid = 1;
        int arrival_time = 0;
        int burst_length = 0;
        int interarrival_time = 0;
        if(strcmp(qs, "RM") == 0)
        {
            int index = 0;
            for(int i=0; i<pc; i++)
            {
                interarrival_time = random_generator(l, l1, l2);
                if(i == 0)
                    interarrival_time = 0;

                arrival_time += interarrival_time;
                burst_length = random_generator(t, t1, t2);

                if(index >= n)
                    index = index%n;


                if(heads[index] == NULL)
                {
                    push(&heads[index], pid, burst_length, arrival_time);
                    tmp[index] = heads[index];
                    pid++;
                }   
                else
                {
                    insert(tmp[index], pid, burst_length, arrival_time);
                    tmp[index] = tmp[index]->next;
                    pid++;
                }
                index++;

            }    
        }  

        if(strcmp(qs, "LM") == 0)
        {
            for(int i=0; i<pc; i++)
            {

                interarrival_time = random_generator(l, l1, l2);
                if(i == 0)
                    interarrival_time = 0;

                arrival_time += interarrival_time;
                burst_length = random_generator(t, t1, t2);


                bool is_inserted = false;
                for(int i=0; i<n; i++)
                {    
                    if(heads[i] == NULL)
                    {
                        push(&heads[i], pid, burst_length, arrival_time);
                        tmp[i] = heads[i];
                        is_inserted = true;
                        pid++;
                        break;
                    }

                }   
                if(!is_inserted)
                {
                    int i = get_min_length(heads, n);    
                    insert(tmp[i], pid, burst_length, arrival_time);
                    tmp[i] = tmp[i]->next;
                    pid++;
                }
            }        
        } 
        

    }    

    //insert dummy item
    if(strcmp(sap, "M") == 0 )
    {
        for(int i=0; i<n; i++)
        {
            if(tmp[i] == NULL)
                insert(tmp[i], -1, 0, 0);  
        }    
    }  

    for(int i = 0; i < n; i++)
    {
        printf("for loop %d\n", i);
        printf("\nnom");
        cpu_ids[i] = i;
        BurstNode* initial = NULL;
        int elapsed_ptime = 0;
        args[i].cpu_id = i;
        args[i].current_time = 0;
        args[i].alg = alg;
        args[i].sap = sap;
        args[i].qs = qs;
        args[i].q = q;
        args[i].outmode = outmode;

        if(strcmp(sap, "S") == 0)
            args[i].burst = head;
        else if(strcmp(sap, "M") == 0)
            args[i].burst = heads[i];

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

    if(outmode == 2 && strcmp(sap, "S") == 0)
        printBursts(head);

    if(outmode == 2 && strcmp(sap, "M") == 0)
    {
        for(int i=0; i<n; i++)
        {
            printBursts(heads[i]);
        }    
    }    


    return 0;
}

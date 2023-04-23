#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
        printf("pid %d ", temp->pid);
        printf("length %d ", temp->burst_length);
        temp = temp->next;
    }
}

/*
Burst* create_burst(int pid, int length, int arrival_time)
{
    Burst* burst = (Burst*) malloc(sizeof(Burst));
    burst->pid = pid;
    burst->burst_length = length;
    burst->arrival_time = arrival_time;
    burst->remaining_time = length;
    burst->finish_time = 0;
    burst->turnaround_time = 0;
    burst->cpu_id = 0;
    burst->next = NULL;
    return burst;
}*/

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
    BurstNode* head = NULL;
    BurstNode* temp = NULL;
    int burst_length, arrival_time;

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

        printf("Retrieved line of length %zu:\n", read);
        printf("%s", line);

        strcpy(lie, line);
        token = strtok(lie, " ");

        if (token != NULL && strcmp(token, "PL") == 0)
        {
            printf("First word: %s\n", token);
            token = strtok(NULL, " ");
            printf("Length: %s\n", token);
            burst_length = atoi(token);
        }

        if (token != NULL && strcmp(token, "IAT") == 0)
        {
            printf("First word: %s\n", token);
            token = strtok(NULL, " ");
            printf("Arrival time: %s\n", token);
            arrival_time = atoi(token);
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
    }    
    fclose(fp);
    if (line)
        free(line);

    printList(head);

    gettimeofday(&current_time, NULL);
    long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
    long elapsed_ms = current_ms - start_ms;
    printf("Elapsed time: %ld ms\n", elapsed_ms);

    return 0;
}

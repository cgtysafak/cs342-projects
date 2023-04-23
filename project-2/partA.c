#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    int num_procs = 2;
    char sched_approach = 'M';
    char queue_sel = 'R';
    char sched_alg = 'R';
    int time_quantum = 20;
    char *infile = "in.txt";
    int out_mode = 1;
    char *outfile = "out.txt";
    int random = 0;
    int t = 0, t1 = 0, t2 = 0, l = 0, l1 = 0, l2 = 0;
    int opt;

    while ((opt = getopt(argc, argv, "n:a:s:i:m:o:r:")) != -1) {
        switch (opt) {
            case 'n':
                num_procs = atoi(optarg);
                break;
            case 'a':
                sched_approach = optarg[0];
                queue_sel = optarg[1];
                break;
            case 's':
                sched_alg = optarg[0];
                time_quantum = atoi(optarg + 1);
                break;
            case 'i':
                infile = optarg;
                break;
            case 'm':
                out_mode = atoi(optarg);
                break;
            case 'o':
                outfile = optarg;
                break;
            case 'r':
                random = 1;
                sscanf(optarg, "%d %d %d %d %d %d", &t, &t1, &t2, &l, &l1, &l2);
                break;
            default:
                fprintf(stderr, "Usage: %s [-n N] [-a SAP QS] [-s ALG Q] [-i INFILE] [-m OUTMODE] [-o OUTFILE] [-r T T1 T2 L L1 L2]\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    printf("num_procs=%d, sched_approach=%c, queue_sel=%c, sched_alg=%c, time_quantum=%d, infile=%s, out_mode=%d, outfile=%s, random=%d, t=%d, t1=%d, t2=%d, l=%d, l1=%d, l2=%d\n",
           num_procs, sched_approach, queue_sel, sched_alg, time_quantum, infile, out_mode, outfile, random, t, t1, t2, l, l1, l2);

    struct timeval start_time, current_time;
    gettimeofday(&start_time, NULL);
    long start_ms = start_time.tv_sec * 1000 + start_time.tv_usec / 1000;

    // Do some work here...

    gettimeofday(&current_time, NULL);
    long current_ms = current_time.tv_sec * 1000 + current_time.tv_usec / 1000;
    long elapsed_ms = current_ms - start_ms;
    printf("Elapsed time: %ld ms\n", elapsed_ms);

    return 0;
}
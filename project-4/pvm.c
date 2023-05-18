#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>

int main(int argc, char *argv[])
{
    int opt;
	char *command = "";
	
	if (argc > 2)
	{
        command = argv[1];
	}
	
	if (strcmp(command, "-freefc") == 0)
	{
        if (argc != 4) {
            printf("Usage: %s -freefc <PFN1> <PFN2>\n", argv[0]);
            return 1;
        }
        
        unsigned long PFN1 = strtoul(argv[2], NULL, 0);
        unsigned long PFN2 = strtoul(argv[3], NULL, 0);
        
        // Handle -freefc command with PFN1 and PFN2
        printf("Command: -freefc\nPFN1: %lu\nPFN2: %lu\n", PFN1, PFN2);
		
        // TODO
    }
	else if (strcmp(command, "-frameinto") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -frameinto <PFN>\n", argv[0]);
            return 1;
        }
        
        unsigned long PFN = strtoul(argv[2], NULL, 0);
        
        // Handle -frameinto command with PFN
        printf("Command: -frameinto\nPFN: %lu\n", PFN);
		
        // TODO
    }
	else if (strcmp(command, "-memused") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -memused <PID>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        
        // Handle -memused command with PID
        printf("Command: -memused\nPID: %lu\n", PID);
		
        // TODO
    }
	else if (strcmp(command, "-mapva") == 0)
	{
        if (argc != 4) {
            printf("Usage: %s -mapva <PID> <VA>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        unsigned long VA = strtoul(argv[3], NULL, 0);
        
        // Handle -mapva command with PID
        printf("Command: -mapva\nPID: %lu\nVA: %lu\n", PID, VA);
		
        // TODO
    }
	else if (strcmp(command, "-pte") == 0)
	{
        if (argc != 4) {
            printf("Usage: %s -pte <PID> <VA>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        unsigned long VA = strtoul(argv[3], NULL, 0);
        
        // Handle -pte command with PID
        printf("Command: -pte\nPID: %lu\nVA: %lu\n", PID, VA);
		
        // TODO
    }
	else if (strcmp(command, "-maprange") == 0)
	{
        if (argc != 5) {
            printf("Usage: %s -maprange <PID> <VA1> <VA2>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        unsigned long VA1 = strtoul(argv[3], NULL, 0);
        unsigned long VA2 = strtoul(argv[4], NULL, 0);
        
        // Handle -maprange command with PID
        printf("Command: -maprange\nPID: %lu\nVA1: %lu\nVA2: %lu\n", PID, VA1, VA2);
		
        // TODO
	}
	else if (strcmp(command, "-mapall") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -mapall <PID>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        
        // Handle -mapall command with PID
        printf("Command: -mapall\nPID: %lu\n", PID);
		
        // TODO
    }
	else if (strcmp(command, "-mapallin") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -mapallin <PID>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        
        // Handle -mapallin command with PID
        printf("Command: -mapallin\nPID: %lu\n", PID);
		
        // TODO
    }
	else if (strcmp(command, "-alltablesize") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -alltablesize <PID>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        
        // Handle -alltablesize command with PID
        printf("Command: -alltablesize\nPID: %lu\n", PID);
		
        // TODO
    }

    printf("end");
    
    return 0;
}

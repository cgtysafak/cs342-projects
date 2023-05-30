#include <stdio.h>
#include <sys/time.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <pthread.h>
#include <math.h>
#include <fcntl.h>

#include <stdint.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PFN_MASK ((1ULL << 55) - 1)
#define PAGE_SIZE 4096

int main(int argc, char *argv[])
{
	char *command = "";
	
	if (argc > 2)
	{
        command = argv[1];
        printf("Command: %s\n", command);
        printf("argc: %d\n", argc);
	}

	if (strcmp(command, "-frameinfo") == 0)
	{

        // if (argc != 3) {
        //     printf("Usage: %s -frameinto <PFN>\n", argv[0]);
        //     return 1;
        // }
        //int fd, fd2;

        unsigned long PFN = strtoul(argv[2], NULL, 0);
        printf("%lu", PFN);
    }
	else if (strcmp(command, "-memused") == 0)
    {
        if (argc != 3) {
            printf("Usage: %s -memused <PID>\n", argv[0]);
            return 1;
        }

        // get the PID
        unsigned long PID = strtoul(argv[2], NULL, 0);

        // read the maps file
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "/proc/%lu/maps", PID);
        FILE *file = fopen(filePath, "r");

        // maps check
        if(file == NULL) {
            printf("No such file %s\n", filePath);
            return 1;
        }

        // read the kpagecount file
        // int kpagecountFile = open("/proc/kpagecount", O_RDONLY);
        
        // // maps check
        // if (kpagecountFile == -1) {
        //     printf("Unable to open the /proc/kpagecount file.\n");
        //     return 1;
        // }

        // calculate the memory usage line by line
        char line[256];
        unsigned long totalMemoryUsage = 0;
        unsigned long totalPhysUsage = 0;
        unsigned long exclusivePhysUsage = 0;
        while (fgets(line, sizeof(line), file) != NULL)
        {
            unsigned long startAddress, endAddress, entry, offset;

            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &startAddress, &endAddress);

            // Calculate the virtual memory usage for the line
            unsigned long memoryUsage = endAddress - startAddress;

            totalMemoryUsage += memoryUsage;

            // Calculate the PFN from VPN
            unsigned long VPN = startAddress / 4096;

            char pagemapPath[256];
            snprintf(pagemapPath, sizeof(pagemapPath), "/proc/%lu/pagemap", PID);
            
            int fd = open(pagemapPath, O_RDONLY);
            if(fd == -1 ) {
                perror("Error opening pagemap");
                return 1;
            }

            offset = sizeof(unsigned long) * VPN;
            lseek(fd, offset, SEEK_CUR);
            read(fd, &entry, sizeof(unsigned long));

            unsigned long mask_PFN = (1UL << 55)-1;
            unsigned long PFN = entry & mask_PFN;
            unsigned long present_bit = (entry >> 63) & 1;

            // printf("VPN = %lu\tEntry = %lu\tPFN = %lu\tpresent = %lu\n", VPN, entry, PFN, present_bit);

            // AAAAAAAAAAAAAAAAAAAAAAA
            int fd2 = open("/proc/kpagecount", O_RDONLY);
			if(fd2 == -1 ) {
			    perror("Error opening /proc/kpagecount");
			    return 1;
			}

			off_t offset2 = 8*PFN;
		    uint64_t entry2;

            lseek(fd2, offset2, SEEK_SET);
            read(fd2, &entry2, sizeof(unsigned long));

            if (entry2 == 1 && present_bit != 0)
            	exclusivePhysUsage += memoryUsage;
            if (entry2 >= 1 && present_bit != 0)
            	totalPhysUsage += memoryUsage;

            // printf("%s\n", line);
        }

        printf("Total virtual memory usage: %lu KBs\n", (totalMemoryUsage/1024));
        printf("Total physical memory usage: %lu KBs\n", (totalPhysUsage/1024));
        printf("Exclusive physical memory usage: %lu KBs\n", (exclusivePhysUsage/1024));

        fclose(file);

        // ------------------------
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

    printf("end\n");
    
    return 0;
}

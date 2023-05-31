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
#define ENTRY_SIZE 8 // 64 bits
#define LEVEL_OFFSET_BITS 9
#define LEVEL_ENTRIES (1ULL << LEVEL_OFFSET_BITS) //1000000000 - 1 = 0111111111

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
            unsigned long PFN = strtoul(argv[2], NULL, 0);
            printf("For frame 0x%016lX:\n\n", PFN);

            int fd = open("/proc/kpageflags", O_RDONLY);
	    if(fd == -1 ) {
	        perror("Error opening /proc/kpageflags");
	        return 1;
	    }
	    
	    //constant string array for flag names
	    const char* flag_names[26] = { "LOCKED", "ERROR", "REFERENCED","UPTODATE",  "DIRTY",           
		                               "LRU"   , "ACTIVE", "SLAB",     "WRITEBACK", "RECLAIM",         
		                               "BUDDY" , "MMAP",   "ANON",     "SWAPCACHE", "SWAPBACKED",      
		                               "COMPOUND_HEAD", "COMPOUND_TAIL", "HUGE", "UNEVICTABLE", "HWPOISON",        
		                               "NOPAGE", "KSM", "THP", "BALLOON", "ZERO_PAGE",      
		                               "IDLE" };
	    
	    off_t offset = 8*PFN;
	    uint64_t entry;
	    
	    uint64_t mask = (1ULL << 26) -1; //to get least 26 significant bits
	    uint64_t last_26_bits = 0;
	    

	    if(lseek(fd, offset, SEEK_CUR) == -1)
	    {
	    	perror("Error seeking in /proc/pageflags");
	    	close(fd);
	    	return 1;
	    }

	    if(read(fd, &entry, sizeof(uint64_t)) != sizeof(uint64_t)) 
	    {
	    	perror("Error reading /proc/pageflags");
	    	close(fd);
	    	return 1;
	    }

	    last_26_bits = entry & mask;

	    //i represents bit position
	    for (int i = 0; i < 26; i++) {
	        uint64_t mask = 1ULL << i;  

	        uint64_t bit = (last_26_bits & mask) >> i; 

	        printf("%d.%s: %lu\n", i, flag_names[i], bit); //print each bit for flag values respectively
	    }
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

        // Open the maps file
        char maps_file_path[256];
        snprintf(maps_file_path, sizeof(maps_file_path), "/proc/%lu/maps", PID);

        FILE *maps_file = fopen(maps_file_path, "r");
        if (maps_file == NULL) {
            perror("Cannot open maps file");
            return 1;
        }

        unsigned long start_address, end_address;

        char line[256];
        while (fgets(line, sizeof(line), maps_file) != NULL) {
            // printf("\n line %s", line);
            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &start_address, &end_address);
            if (VA >= start_address && VA < end_address) {
                break;
            }
        }

        fclose(maps_file);

        unsigned long long VPN = start_address / PAGE_SIZE;

        // Open pagemap file
        char pagemap_file_path[256];
        snprintf(pagemap_file_path, sizeof(pagemap_file_path), "/proc/%lu/pagemap", PID);

        int pagemap_file = open(pagemap_file_path, O_RDONLY);
        if (pagemap_file < 0) {
            perror("Cannot open pagemap file");
            return 1;
        }

        unsigned long offset = VPN * ENTRY_SIZE;
        // Read the entries from the pagemap file
        uint64_t entry;
        lseek(pagemap_file, offset, SEEK_CUR);
        read(pagemap_file, &entry, ENTRY_SIZE);

        unsigned long long PFN = entry & 0x7FFFFFFFFFFFFF;

        unsigned long long physical_address = (PFN * PAGE_SIZE) + (VA % PAGE_SIZE);  

        printf("\nPA: 0x%016llx\n", physical_address);
        printf("\nPFN: 0x%09llx\n", PFN);

        return 0;
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

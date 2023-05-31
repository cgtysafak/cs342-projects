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
        FILE *file_ctrl = fopen(filePath, "r");

        // maps check
        if(file == NULL) {
            printf("No such file %s\n", filePath);
            return 1;
        }

        // calculate the memory usage line by line
        char line[256];
        int noOfLines = 0;
        unsigned long totalMemoryUsage = 0;
        unsigned long totalPhysUsage = 0;
        unsigned long exclusivePhysUsage = 0;

        while (fgets(line, sizeof(line), file_ctrl) != NULL) {noOfLines++;}

		printf("NO OF LINES = %d\n\n", noOfLines);

		int ctrl = 0;
        while (fgets(line, sizeof(line), file) != NULL)
        {
        	++ctrl;
            unsigned long startAddress, endAddress, entry_pagemap, entry_kpagecount, offset_pagemap, offset_kpagecount;

            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &startAddress, &endAddress);

            // Calculate the virtual memory usage for the line
            unsigned long memoryUsage = endAddress - startAddress;

            totalMemoryUsage += memoryUsage;

            if (ctrl == noOfLines)
            	break;

            // Calculate the PFN from VPN
            unsigned long VPN_start = startAddress / 4096;
            unsigned long VPN_end = endAddress / 4096;

            char pagemapPath[256];
            snprintf(pagemapPath, sizeof(pagemapPath), "/proc/%lu/pagemap", PID);
            
            int pagemap = open(pagemapPath, O_RDONLY);
            if(pagemap == -1 ) {
                perror("Error opening pagemap");
                return 1;
            }

			int kpagecount = open("/proc/kpagecount", O_RDONLY);
			if(kpagecount == -1 ) {
			    perror("Error opening /proc/kpagecount");
			    return 1;
			}

            for(unsigned long i = VPN_start; i <= VPN_end; ++i)
            {
				offset_pagemap = sizeof(unsigned long) * i;
				lseek(pagemap, offset_pagemap, SEEK_CUR);
				read(pagemap, &entry_pagemap, sizeof(unsigned long));

				unsigned long mask_PFN = (1UL << 55)-1;
				unsigned long PFN = entry_pagemap & mask_PFN;
				unsigned long present_bit = (entry_pagemap >> 63) & 1;

				if (present_bit == 0)
					continue;

				// printf("VPN_start = %lu\tVPN_end = %lu\tEntry = %lu\tPFN = %lu\tpresent = %lu\n", VPN_start, VPN_end, entry_pagemap, PFN, present_bit);

				offset_kpagecount = 8*PFN;

				lseek(kpagecount, offset_kpagecount, SEEK_CUR);
				read(kpagecount, &entry_kpagecount, sizeof(unsigned long));

				if (entry_kpagecount == 1 && present_bit != 0)
					exclusivePhysUsage += 4096;
				if (entry_kpagecount >= 1 && present_bit != 0)
					totalPhysUsage += 4096;
            }

            // printf("%s\n", line);
        }

        printf("Total virtual memory usage: %lu KB\n", (totalMemoryUsage/1024));
        printf("Total physical memory usage: %lu KB\n", (totalPhysUsage/1024));
        printf("Exclusive physical memory usage: %lu KB\n", (exclusivePhysUsage/1024));

        fclose(file);
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

        // read the maps file
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "/proc/%lu/maps", PID);
        FILE *file = fopen(filePath, "r");

        // maps check
        if(file == NULL) {
            printf("No such file %s\n", filePath);
            return 1;
        }

        unsigned long VPN_VA1 = VA1 / 4096;
        unsigned long VPN_VA2 = VA2 / 4096;

        for(unsigned long i = VPN_VA1; i < VPN_VA2; ++i)
        {
        	char *result = "unused";
        	unsigned long result_VPN = i;
        	unsigned long result_PFN = 0;
        	bool in_memory = false;
        	char line[256];
	        while ((fgets(line, sizeof(line), file) != NULL) && !in_memory)
	        {
	        	unsigned long startAddress, endAddress, entry_pagemap, entry_kpagecount, offset_pagemap, offset_kpagecount;

	        	// Extract start and end addresses from the line
	        	sscanf(line, "%lx-%lx", &startAddress, &endAddress);

	            // Calculate the PFN from VPN
	            unsigned long VPN_start = startAddress / 4096;
	            unsigned long VPN_end = endAddress / 4096;

	            if(i < VPN_start || i > VPN_end)
	            {
	            	// continue;
	            }
	            else
	            {
	            	char pagemapPath[256];
		            snprintf(pagemapPath, sizeof(pagemapPath), "/proc/%lu/pagemap", PID);

		            int pagemap = open(pagemapPath, O_RDONLY);
		            if(pagemap == -1 ) {
		                perror("Error opening pagemap");
		                return 1;
		            }

					int kpagecount = open("/proc/kpagecount", O_RDONLY);
					if(kpagecount == -1 ) {
					    perror("Error opening /proc/kpagecount");
					    return 1;
					}

					offset_pagemap = sizeof(unsigned long) * i;
					lseek(pagemap, offset_pagemap, SEEK_CUR);
					read(pagemap, &entry_pagemap, sizeof(unsigned long));

					unsigned long mask_PFN = (1UL << 55)-1;
					unsigned long PFN = entry_pagemap & mask_PFN;
					unsigned long present_bit = (entry_pagemap >> 63) & 1;

					if (present_bit == 0)
					{
						result = "not-in-memory";
					}
					else
					{
						offset_kpagecount = 8*PFN;
					
						lseek(kpagecount, offset_kpagecount, SEEK_CUR);
						read(kpagecount, &entry_kpagecount, sizeof(unsigned long));
	
						if (entry_kpagecount == 0)
							result = "not-in-memory";
						else
						{
							in_memory = true;
							result_PFN = PFN;
						}
					}
	            }
	        }
			
			printf("VPN = %lu PFN = %lu %s\n", result_VPN, result_PFN, result);
        }
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
		
		// read the file
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "/proc/%lu/maps", PID);
        FILE* file = fopen(filePath, "r");

        // maps file check
        if (file == NULL) {
            printf("ERROR: File not found. Path: %s\n", filePath);
            return 1;
        }

		uintptr_t start_addr, end_addr;
		unsigned long long offset;
		char permissions[5];
		int dev_major, dev_minor, inode;
		char pathname[256];

		while (fscanf(file, "%lx-%lx %s %llx %x:%x %d",
			&start_addr,
			&end_addr,
			permissions,
			&offset,
			&dev_major,
			&dev_minor,
			&inode
		) != EOF) {
        	char *result = "unused";
        	unsigned long result_VPN = i;
        	unsigned long result_PFN = 0;
        	bool in_memory = false;
        	char line[256];
	        while ((fgets(line, sizeof(line), file) != NULL) && !in_memory) {
	        	unsigned long startAddress, endAddress, entry_pagemap, entry_kpagecount, offset_pagemap, offset_kpagecount;

	        	// Extract start and end addresses from the line
	        	sscanf(line, "%lx-%lx", &startAddress, &endAddress);

	            // Calculate the PFN from VPN
	            unsigned long VPN_start = startAddress / 4096;
	            unsigned long VPN_end = endAddress / 4096;

	            if (i < VPN_start || i > VPN_end) {
	            	// continue;
	            }
	            else {
	            	char pagemapPath[256];
		            snprintf(pagemapPath, sizeof(pagemapPath), "/proc/%lu/pagemap", PID);

		            int pagemap = open(pagemapPath, O_RDONLY);
		            if(pagemap == -1 ) {
		                perror("ERROR: Could not open pagemap");
		                return 1;
		            }

					int kpagecount = open("/proc/kpagecount", O_RDONLY);
					if(kpagecount == -1 ) {
					    perror("ERROR: Could not open /proc/kpagecount");
					    return 1;
					}

					offset_pagemap = sizeof(unsigned long) * i;
					lseek(pagemap, offset_pagemap, SEEK_CUR);
					read(pagemap, &entry_pagemap, sizeof(unsigned long));

					unsigned long mask_PFN = (1UL << 55)-1;
					unsigned long PFN = entry_pagemap & mask_PFN;
					unsigned long present_bit = (entry_pagemap >> 63) & 1;

					if (present_bit == 0)
					{
						result = "not-in-memory";
					}
					else
					{
						offset_kpagecount = 8*PFN;
					
						lseek(kpagecount, offset_kpagecount, SEEK_CUR);
						read(kpagecount, &entry_kpagecount, sizeof(unsigned long));
	
						if (entry_kpagecount == 0)
							result = "not-in-memory";
						else
						{
							in_memory = true;
							result_PFN = PFN;
						}
					}
	            }
	        }
			
			printf("VPN = %lu PFN = %lu %s\n", result_VPN, result_PFN, result);
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

/*
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

// Function to read the content of a binary file
void read_binary_file(const char *filename, void *buffer, size_t size) {
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        perror("Error opening file");
        exit(1);
    }

    ssize_t num_read = read(fd, buffer, size);
    if (num_read < 0) {
        perror("Error reading file");
        exit(1);
    }

    close(fd);
}

// Function to calculate the physical address from the virtual address
uintptr_t calculate_physical_address(pid_t pid, uintptr_t virtual_address) {
    char pagemap_file[64];
    uintptr_t page_frame_number;
    off_t offset;

    sprintf(pagemap_file, "/proc/%d/pagemap", pid);
    int fd = open(pagemap_file, O_RDONLY);
    if (fd < 0) {
        perror("Error opening pagemap file");
        exit(1);
    }

    offset = (virtual_address / PAGE_SIZE) * sizeof(uint64_t);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to pagemap entry");
        exit(1);
    }

    uint64_t pagemap_entry;
    if (read(fd, &pagemap_entry, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("Error reading pagemap entry");
        exit(1);
    }

    page_frame_number = pagemap_entry & 0x7FFFFFFFFFFFFF;
    close(fd);

    return (page_frame_number * PAGE_SIZE) + (virtual_address % PAGE_SIZE);
}

// Function to print the (page number, frame number) mappings for all used pages
void print_page_frame_mappings(pid_t pid, int in_memory_only) {
    char maps_file[64];
    char pagemap_file[64];

    sprintf(maps_file, "/proc/%d/maps", pid);
    sprintf(pagemap_file, "/proc/%d/pagemap", pid);

    FILE *maps_fp = fopen(maps_file, "r");
    if (maps_fp == NULL) {
        perror("Error opening maps file");
        exit(1);
    }

    int pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
        perror("Error opening pagemap file");
        exit(1);
    }

    uintptr_t start_addr, end_addr;
    unsigned long long offset;
    char permissions[5];
    int dev_major, dev_minor, inode;
    char pathname[256];

    while (fscanf(maps_fp, "%lx-%lx %s %llx %x:%x %d",
                  &start_addr, &end_addr, permissions, &offset,
                  &dev_major, &dev_minor, &inode) != EOF) {
        if (fgets(pathname, sizeof(pathname), maps_fp) == NULL) {
            perror("Error reading pathname from maps file");
            exit(1);
        }

        // Remove newline character from the end of the pathname
        size_t len = strlen(pathname);
        if (len > 0 && pathname[len - 1] == '\n') {
            pathname[len - 1] = '\0';
        }

        printf("Mapping: %lx-%lx %s\n", start_addr, end_addr, pathname);

        uintptr_t virtual_address = start_addr;
        while (virtual_address < end_addr) {
            off_t offset = (virtual_address / PAGE_SIZE) * sizeof(uint64_t);
            if (lseek(pagemap_fd, offset, SEEK_SET) == -1) {
                perror("Error seeking to pagemap entry");
                exit(1);
            }

            uint64_t pagemap_entry;
            if (read(pagemap_fd, &pagemap_entry, sizeof(uint64_t)) != sizeof(uint64_t)) {
                perror("Error reading pagemap entry");
                exit(1);
            }

            if (!in_memory_only || (pagemap_entry >> 63) & 1) {
                uintptr_t page_frame_number = pagemap_entry & 0x7FFFFFFFFFFFFF;
                printf("Virtual Address: %lx -> Physical Address: %lx\n", virtual_address,
                       (page_frame_number * PAGE_SIZE) + (virtual_address % PAGE_SIZE));
            }

            virtual_address += PAGE_SIZE;
        }
    }

    fclose(maps_fp);
    close(pagemap_fd);
}

int main(int argc, char *argv[]) {
    pid_t pid = getpid();

    printf("Page-Frame Mappings for Used Pages:\n");
    print_page_frame_mappings(pid, 0);

    printf("\nPage-Frame Mappings for Used Pages in Memory:\n");
    print_page_frame_mappings(pid, 1);

    return 0;
}

------------------------------------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

#define PAGE_SIZE 4096
#define PAGE_SHIFT 12

// Function to read the content of a binary file
void read_binary_file(const char *filename, void *buffer, size_t size) {
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Error opening file");
        exit(1);
    }

    size_t num_read = fread(buffer, 1, size, file);
    if (num_read != size) {
        perror("Error reading file");
        exit(1);
    }

    fclose(file);
}

// Function to calculate the physical address from the virtual address
uintptr_t calculate_physical_address(pid_t pid, uintptr_t virtual_address) {
    char pagemap_file[64];
    uintptr_t page_frame_number;
    off_t offset;

    sprintf(pagemap_file, "/proc/%d/pagemap", pid);
    int fd = open(pagemap_file, O_RDONLY);
    if (fd < 0) {
        perror("Error opening pagemap file");
        exit(1);
    }

    offset = (virtual_address / PAGE_SIZE) * sizeof(uint64_t);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to pagemap entry");
        exit(1);
    }

    uint64_t pagemap_entry;
    if (read(fd, &pagemap_entry, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("Error reading pagemap entry");
        exit(1);
    }

    page_frame_number = pagemap_entry & 0x7FFFFFFFFFFFFF;
    close(fd);

    return (page_frame_number * PAGE_SIZE) + (virtual_address % PAGE_SIZE);
}

// Function to check if a page is in memory
int is_page_in_memory(pid_t pid, uintptr_t virtual_address) {
    char pagemap_file[64];
    off_t offset;

    sprintf(pagemap_file, "/proc/%d/pagemap", pid);
    int fd = open(pagemap_file, O_RDONLY);
    if (fd < 0) {
        perror("Error opening pagemap file");
        exit(1);
    }

    offset = (virtual_address / PAGE_SIZE) * sizeof(uint64_t);
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Error seeking to pagemap entry");
        exit(1);
    }

    uint64_t pagemap_entry;
    if (read(fd, &pagemap_entry, sizeof(uint64_t)) != sizeof(uint64_t)) {
        perror("Error reading pagemap entry");
        exit(1);
    }

    close(fd);

    return (pagemap_entry >> 63) & 1;
}

// Function to print the (page number, frame number) mappings for all used pages
void print_page_frame_mappings(pid_t pid) {
    char maps_file[64];
    char pagemap_file[64];

    sprintf(maps_file, "/proc/%d/maps", pid);
    sprintf(pagemap_file, "/proc/%d/pagemap", pid);

    FILE *maps_fp = fopen(maps_file, "r");
    if (maps_fp == NULL) {
        perror("Error opening maps file");
        exit(1);
    }

    int pagemap_fd = open(pagemap_file, O_RDONLY);
    if (pagemap_fd < 0) {
        perror("Error opening pagemap file");
        exit(1);
    }

    uintptr_t start_addr, end_addr;
    char perms[5], pathname[1000];
    int in_memory;

    while (fscanf(maps_fp, "%lx-%lx %4s %*lx %*x:%*x %*d%*[ \t]%[^\n]", &start_addr, &end_addr, perms, pathname) != EOF) {
        in_memory = is_page_in_memory(pid, start_addr);

        if (strcmp(perms, "r--p") != 0 || !in_memory) {
            printf("Virtual Address: %lx -> Not in memory\n", start_addr);
        } else {
            uintptr_t page_frame_number = calculate_physical_address(pid, start_addr) >> PAGE_SHIFT;
            printf("Virtual Address: %lx -> Physical Page Frame Number: %lx\n", start_addr, page_frame_number);
        }

        if (pathname[strlen(pathname) - 1] == '\n') {
            pathname[strlen(pathname) - 1] = '\0';
        }

        printf("Mapping: %lx-%lx %s\n", start_addr, end_addr, pathname);
    }

    fclose(maps_fp);
    close(pagemap_fd);
}

int main() {
    pid_t pid = getpid();

    printf("Page-Frame Mappings for Used Pages:\n");
    print_page_frame_mappings(pid);

    return 0;
}
*/
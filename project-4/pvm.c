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
	}

	if (strcmp(command, "-frameinfo") == 0)
	{
            unsigned long PFN = strtoul(argv[2], NULL, 0);
            printf("For frame 0x%016lX:\n\n", PFN);

            int fd = open("/proc/kpageflags", O_RDONLY);
	    if(fd == -1 ) {
	        perror("ERROR: could not open: /proc/kpageflags");
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
	    	perror("ERROR: could not seek: /proc/pageflags");
	    	close(fd);
	    	return 1;
	    }

	    if(read(fd, &entry, sizeof(uint64_t)) != sizeof(uint64_t)) 
	    {
	    	perror("EERROR: could not read: /proc/pageflags");
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
                perror("ERROR: could not open: pagemap");
                return 1;
            }

			int kpagecount = open("/proc/kpagecount", O_RDONLY);
			if(kpagecount == -1 ) {
			    perror("ERROR: could not open: /proc/kpagecount");
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

				offset_kpagecount = 8*PFN;

				lseek(kpagecount, offset_kpagecount, SEEK_CUR);
				read(kpagecount, &entry_kpagecount, sizeof(unsigned long));

				if (entry_kpagecount == 1 && present_bit != 0)
					exclusivePhysUsage += 4096;
				if (entry_kpagecount >= 1 && present_bit != 0)
					totalPhysUsage += 4096;
            }
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
        // Open the maps file
        char maps_file_path[256];
        snprintf(maps_file_path, sizeof(maps_file_path), "/proc/%lu/maps", PID);

        FILE *maps_file = fopen(maps_file_path, "r");
        if (maps_file == NULL) {
            perror("ERROR: Cannot open maps file");
            return 1;
        }

        unsigned long start_address, end_address;

        char line[256];
        while (fgets(line, sizeof(line), maps_file) != NULL) {
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
            perror("ERROR: Cannot open pagemap file");
            return 1;
        }

        unsigned long offset = VPN * ENTRY_SIZE;
        // Read the entries from the pagemap file
        uint64_t entry;
        lseek(pagemap_file, offset, SEEK_CUR);
        read(pagemap_file, &entry, ENTRY_SIZE);

        unsigned long present_bit = (entry >> 63) & 1;
        if (present_bit == 0) {
            printf("\nnot-in-memory");
        } else {
            unsigned long long PFN = entry & 0x7FFFFFFFFFFFFF;

            unsigned long long physical_address = (PFN * PAGE_SIZE) + (VA % PAGE_SIZE);  

            printf("\nPA: 0x%016llx", physical_address);
            printf("\nPFN: 0x%09llx\n", PFN);
        }

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

        // read the maps file
        char filePath[256];
        snprintf(filePath, sizeof(filePath), "/proc/%lu/maps", PID);
        FILE *file = fopen(filePath, "r");

        // maps check
        if(file == NULL) {
            printf("No such file %s\n", filePath);
            return 1;
        }
        char line[256];

        while (fgets(line, sizeof(line), file) != NULL)
        {
            unsigned long startAddress, endAddress, entry, offset;

            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &startAddress, &endAddress);

            if(VA >= startAddress && VA <= endAddress)
            {
                unsigned long VPN = VA / 4096;

                char pagemapPath[256];
                snprintf(pagemapPath, sizeof(pagemapPath), "/proc/%lu/pagemap", PID);
                
                int fd = open(pagemapPath, O_RDONLY);
                if(fd == -1 ) {
                    perror("ERROR: could not open: pagemap");
                    return 1;
                }
   
                offset = sizeof(unsigned long) * (VPN);
                lseek(fd, offset, SEEK_SET);
                read(fd, &entry, sizeof(unsigned long));

                unsigned long mask_PFN = (1UL << 55)-1;
                unsigned long PFN = entry & mask_PFN;
                unsigned long present_bit = (entry >> 63) & 1;
                unsigned long swapped_bit = (entry >> 62) & 1;
                unsigned long file_page_bit = (entry >> 61) & 1;
                unsigned long soft_dirty_bit = (entry >> 55) & 1;
                
                printf("VA: 0x%016lX\tVPN: 0x%016lX\t", VA, VPN);
                printf("Present bit: %lu\t Swapped bit: %lu\t Filepage bit: %lu\t Softdirty bit: %lu\n", present_bit, swapped_bit, file_page_bit, soft_dirty_bit);
                printf("PFN: 0x%016lX\n", PFN);

                return 0;      
            }    

        }    

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
		                perror("ERROR: could not open: pagemap");
		                return 1;
		            }

					int kpagecount = open("/proc/kpagecount", O_RDONLY);
					if(kpagecount == -1 ) {
					    perror("ERROR: could not open: /proc/kpagecount");
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
		
		if (in_memory)
                	printf("VPN = 0x%016lX PFN = 0x%016lX\n", result_VPN, result_PFN);
            	else
                	printf("VPN = 0x%016lX %s\n", result_VPN, result);
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
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "/proc/%lu/maps", PID);
        FILE* file = fopen(file_path, "r");

        if (file == NULL) {
            printf("ERROR: File could not be opened. Path: %s\n", file_path);
            return 1;
        }

        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) {
            unsigned long start_address, end_address, vpn, pfn;
            bool in_memory = false;

            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &start_address, &end_address);

            // Calculate the VPN range
            unsigned long vpn_start = start_address / PAGE_SIZE;
            unsigned long vpn_end = end_address / PAGE_SIZE;

            for (vpn = vpn_start; vpn < vpn_end; vpn++) {
                char pagemap_path[256];
                snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%lu/pagemap", PID);

                int pagemap = open(pagemap_path, O_RDONLY);
                if (pagemap == -1) {
                    perror("ERROR: Could not open pagemap");
                    return 1;
                }

                unsigned long offset_pagemap = sizeof(unsigned long) * vpn;
                lseek(pagemap, offset_pagemap, SEEK_SET);
                if (read(pagemap, &pfn, sizeof(unsigned long)) != sizeof(unsigned long)) {
                    return 1;
                }

                unsigned long present_bit = (pfn >> 63) & 1;
                if (present_bit) {
                    in_memory = true;
                    pfn &= ((1UL << 55) - 1);
                }
                else {
                    in_memory = false;
                }

                close(pagemap);

                if (in_memory) {
                    printf("mapping: vpn=0x%lx pfn=0x%lx\n", vpn, pfn);
                }
                else {
                    printf("mapping: vpn=0x%lx not-in-memory\n", vpn);
                }
            }
        }

        fclose(file);
    }
	else if (strcmp(command, "-mapallin") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -mapallin <PID>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        
        // Handle -mapallin command with PID
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "/proc/%lu/maps", PID);
        FILE* file = fopen(file_path, "r");

        if (file == NULL) {
            printf("ERROR: File could not be opened. Path: %s\n", file_path);
            return 1;
        }

        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) {
            unsigned long start_address, end_address, vpn, pfn;

            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &start_address, &end_address);

            // Calculate the VPN range
            unsigned long vpn_start = start_address / PAGE_SIZE;
            unsigned long vpn_end = end_address / PAGE_SIZE;

            for (vpn = vpn_start; vpn < vpn_end; vpn++) {
                char pagemap_path[256];
                snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%lu/pagemap", PID);

                int pagemap = open(pagemap_path, O_RDONLY);
                if (pagemap == -1) {
                    perror("ERROR: Could not open pagemap");
                    return 1;
                }

                unsigned long offset_pagemap = sizeof(unsigned long) * vpn;
                lseek(pagemap, offset_pagemap, SEEK_SET);
                if (read(pagemap, &pfn, sizeof(unsigned long)) != sizeof(unsigned long)) {
                    return 1;
                }

                unsigned long present_bit = (pfn >> 63) & 1;
                if (present_bit) {
                    pfn &= ((1UL << 55) - 1);
                }
                else {
					close(pagemap);
                    continue;
                }

                close(pagemap);
                printf("mapping: vpn=0x%lx pfn=0x%lx\n", vpn, pfn);
            }
        }

        fclose(file);
    }
	else if (strcmp(command, "-alltablesize") == 0)
	{
        if (argc != 3) {
            printf("Usage: %s -alltablesize <PID>\n", argv[0]);
            return 1;
        }
        
        unsigned long PID = strtoul(argv[2], NULL, 0);
        
        // Handle -alltablesize command with PID
        char file_path[256];
        snprintf(file_path, sizeof(file_path), "/proc/%lu/maps", PID);
        FILE* file = fopen(file_path, "r");

        if (file == NULL) {
            printf("ERROR: File could not be opened. Path: %s\n", file_path);
            return 1;
        }

        static int level2[512] = {0};
        static int level3[512][512] = {{0}};
        static int level4[512][512][512] = {{{0}}};

        char line[256];
        while (fgets(line, sizeof(line), file) != NULL) {
			unsigned long start_address, end_address, vpn;

            // Extract start and end addresses from the line
            sscanf(line, "%lx-%lx", &start_address, &end_address);

            // Calculate the VPN range
            unsigned long vpn_start = start_address << 16; // Remove unused part
			vpn_start = vpn_start >> 28; // Recover unused part length + remove page offset

			unsigned long vpn_end = end_address << 16; // Remove unused part
			vpn_end = vpn_end >> 28; // Recover unused part length + remove page offset

			// XXXX XXXX XXXX XXXX 000 000 000, 000 000 000, 000 000 000, 000 000 000, XXXX XXXX XXXX
            for (vpn = vpn_start; vpn < vpn_end; vpn++) {
				unsigned long level2identifier = (vpn >> (9*3)) & 0x1FF;
				unsigned long level3identifier = (vpn >> (9*2)) & 0x1FF;
				unsigned long level4identifier = (vpn >> (9*1)) & 0x1FF;
                
                if (level2identifier > 512 || level3identifier > 512 || level4identifier > 512) {
                    printf("ERROR DETECTED");
                }

				level2[level2identifier] = 1;
				level3[level2identifier][level3identifier] = 1;
				level4[level2identifier][level3identifier][level4identifier] = 1;
            }
		}

		int level1count = 1;
		int level2count = 0;
        int level3count = 0;
        int level4count = 0;

		for (int i = 0; i < 512; i++) {
            if (level2[i] == 1) {
                ++level2count;
                for (int j = 0; j < 512; j++) {
                    if (level3[i][j] == 1) {
                        ++level3count;
                        for (int k = 0; k < 512; k++) {
                            if (level4[i][j][k] == 1) {
                                ++level4count;
                            }
                        }
                    }
                }
            }
		}

		int totalFrames = level1count + level2count + level3count + level4count;
		int totalKB = totalFrames * 4;

		printf("(pid=%lu) total memory occupied by 4-level page table: %d KB (%d frames)\n", PID, totalKB, totalFrames);
		printf("(pid=%lu) number of page tables used: level1=%d, level2=%d, level3=%d, level4=%d\n", PID, level1count, level2count, level3count, level4count);

        fclose(file);
    }
    
    return 0;
}

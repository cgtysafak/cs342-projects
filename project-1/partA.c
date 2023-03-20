/* CS 342 Project 1
   
   Cagatay Safak - 21902730
   Ayse Kelleci - 21902532
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <err.h>

#define STRING_SIZE 64
#define MAX_WORD_SIZE 64

void processFile(int K, int fileIndex, char *fileName, int *word_array_count, char *word_array);

/* Does not work on OS X or macOS, where you can't mmap over /dev/zero */
int main(int argc, char *argv[])
{
	/*	argv[0]  = ./proctopk
	 *	argv[1]  = <K>
	 *	argv[2]  = <output_file>
	 *	argv[3]  = <N>
	 *	argv[4+] = <input_file>
	 */

	int K = atoi(argv[1]);				// <K>
	int N = atoi(argv[3]);				// <N>
	FILE *out_file = fopen(argv[2], "w");		// write only
	//FILE *in_file = fopen(argv[2], "r");		// read only

	// test for files not existing.
	if (out_file == NULL)
	{
		perror("Error! Could not open file.");
		exit(-1);								// idk about exit statements
	}

	int array_size = K;

	int *word_array_count =
		mmap(NULL, N * array_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0); 
	char *word_array =
		mmap(NULL, N * array_size * MAX_WORD_SIZE * sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	if (word_array == MAP_FAILED || word_array_count == MAP_FAILED)
	{
		perror("mmap");
		exit(1);
	}

	// initialize the shared array
	for (int i=0; i<array_size*N; ++i)
	{
		word_array_count[i] = 0;
	}

	for (int i=0; i<N; ++i)
	{
		pid_t pid;

		switch ((pid = fork()))
		{
	        case -1:
				err(1, "fork");
				/* NOTREACHED */
	        case 0:
				FILE *in_file = fopen(argv[ i+4 ], "r");	// read only

				// test for files not existing.
				if (in_file == NULL)
				{
					perror("Error! Could not open file.");
					exit(-1);								// idk about exit statements
				}

				processFile(K, i, argv[ i+4 ], word_array_count, word_array);
				
				return EXIT_SUCCESS;
			default:
				wait(0);
		}
	}

	int total_word = 0;
	char all_words[ N*K ][ MAX_WORD_SIZE ];
	int word_counts[ N*K ];
	char current_word[ MAX_WORD_SIZE ];
	
	// duplicate elimination
	for(int i=0; i<N; ++i) //  0 1
	{
		//int offset = 0;
		for(int j=0; j<K; ++j) // 0 1 2
		{
			//current_word = word_array + offset;
			//offset += strlen(word_array + offset) + 1;
			strcpy(current_word, word_array+(i*K+j)*STRING_SIZE);

			if(strcmp( current_word, "") == 0) //veya NULL konulabilir
			{
				break;
			}

			bool isRepeated = false;
			for(int k=0; k<total_word; ++k)
			{
				if(strcmp(current_word, all_words[k]) == 0)
				{
					word_counts[k] += word_array_count[ i*K + j ];
					isRepeated = true;
				}
			}

			if(!isRepeated)
			{
				strcpy(all_words[total_word], current_word);
				word_counts[total_word] = word_array_count[ i*K + j ];
				++total_word;
			}
		}
	}
	
	// Sorting in descending order
	for (int i=0; i < total_word-1; i++)
	{
		for (int j = i+1; j<total_word; j++)
		{
			int azResult = strcmp(all_words[j], all_words[i]);
			if ( (word_counts[j] > word_counts[i]) || ((word_counts[j] == word_counts[i]) && (azResult < 0)))
			{
				int temp_count = word_counts[i];
				word_counts[i] = word_counts[j];
				word_counts[j] = temp_count;

				char temp_word[ MAX_WORD_SIZE ];
				strcpy(temp_word, all_words[i]);
				strcpy(all_words[i], all_words[j]);
				strcpy(all_words[j], temp_word);
			}
		}
	}

	// write the first K to the outfile
	for(int i=0; i<K; ++i)
	{	
		if(word_array_count[i] != 0)
		{
			fprintf(out_file, "%s %d\n", all_words[i], word_counts[i]);
			//fprintf(out_file, "%s %d\n", word_array+i*STRING_SIZE, word_counts[i]);
		}
	}

	return 0;
}

void processFile(int K, int fileIndex, char *fileName, int *word_array_count, char *word_array)
{
	FILE *in_file = fopen(fileName, "r");		// read only

	// test for files not existing.
	if (in_file == NULL)
	{
		perror("Error! Could not open file.");
		exit(-1);								// idk about exit statements
	}
	else if (K <= 0)
	{
		perror("Error! Could not open file.");
		exit(-1);								// idk about exit statements
	}

	// write to file
	// fprintf(out_file, "this is a test %d\n", 31);

	// read from file
	// int no;
	// fscanf(in_file, "%d", &no);

	// Create an array to store unique words and their counts
	// Initialize dynamic arrays with an initial capacity of 500
	int capacity = 500;
	char** words = (char**) malloc(sizeof(char*) * capacity);
	int* wordCounts = (int*) malloc(sizeof(int) * capacity);

	// Create a variable to keep track of the number of words
	int noOfWords = 0;

	// Read words from file and add them to dynamic arrays
	char word[ MAX_WORD_SIZE ];

	while (fscanf(in_file, "%s", word) == 1)		// &word ya da word emin deiglim + belki fscanf() yerine fgets()
	{
		// Convert the word to upper case
		for (int i=0; i<MAX_WORD_SIZE; ++i)
		{
			word[i] = toupper(word[i]);
		}

		// Check if the word already exists in the array of unique words.
		bool isFound = false;
		for (int i=0; i<noOfWords; ++i)
		{
			if (strcmp(words[i], word) == 0)
			{
				// If the word already exists, increment its count.
				++wordCounts[i];
				isFound = true;

				break;
			}
		}

		// If the word does not exist, add it to the array of unique words.
		if (!isFound)
		{
			// Check if the arrays need to be resized
			if (noOfWords >= capacity)
			{
				capacity *= 2;
				words = (char**) realloc(words, sizeof(char*) * capacity);
				wordCounts = (int*) realloc(wordCounts, sizeof(int) * capacity);
			}

			// Allocate memory for the new word
			words[noOfWords] = (char*) malloc(sizeof(char) * MAX_WORD_SIZE);

			// // Copy the new word into the array and set its count to 1
			// strcpy(words[noOfWords], word);
			// wordCounts[noOfWords] = 1;

			strcpy(words[noOfWords], word);
			++wordCounts[noOfWords++];
		}
	}

	// Sorting in descending order
	for (int i=0; i<noOfWords - 1; ++i)
	{
		for (int j=i + 1; j<noOfWords; ++j)
		{
			int azResult = strcmp(words[j], words[i]);
			if ( (wordCounts[j] > wordCounts[i]) || ((wordCounts[j] == wordCounts[i]) && (azResult < 0)))
			{
				int temp_count = wordCounts[i];
				wordCounts[i] = wordCounts[j];
				wordCounts[j] = temp_count;

				char temp_word[ MAX_WORD_SIZE ];
				strcpy(temp_word, words[i]);
				strcpy(words[i], words[j]);
				strcpy(words[j], temp_word);
			}
		}
	}

	//index ba;langi. noktasi her f'le icin
	int index = fileIndex * K;
	for (int i=0; i<K; ++i)
	{
		word_array_count[ i+index ] = wordCounts[i];
		strcpy(word_array + (i+index)*STRING_SIZE, words[i]);
	}

	// Free the dynamically allocated arrays
	for (int i = 0; i < noOfWords; ++i)
	{
		free(words[i]);
	}
	free(words);
	free(wordCounts);

	fclose(in_file);
}

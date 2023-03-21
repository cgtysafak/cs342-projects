/*	CS 342 Project 1
	Part B
	
	Cagatay Safak	21902730
	Ayse Kelleci	21902532
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <err.h>
#include <pthread.h>

#define MAX_WORD_SIZE 64
#define MAX_FILES 10
#define MAX_FILENAME 256

char filenames[ MAX_FILES ][ MAX_FILENAME ];
int capacity = 1000;	// init
char **words;
int *counts;
int num_words = 0;
int K;
int N;
pthread_mutex_t lock;

void* worker(void *args);

int main(int argc, char *argv[])
{
	/*  argv[0] = ./proctopk
	 *  argv[1] = <K>
	 *  argv[2] = <output_file>
	 *  argv[3] = <N>
	 *  argv[4] = <input_file>1
	 *  argv[5] = <input_file>2
	 *  argv[6] = <input_file>3
	 */

	K = atoi(argv[1]);
	int N = atoi(argv[3]);
	FILE *out_file = fopen(argv[2], "w");		// write only

	// test for invalid inputs and files not existing.
	if (K <= 0)
	{
		perror("Error!!! Invalid value for K.");
		exit(-1);								// idk about exit statements
	}
	else if (N <= 0)
	{
		perror("Error!!! Invalid value for N.");
		exit(-1);								// idk about exit statements
	}
	else if (out_file == NULL)
	{
		perror("Error! No output file.");
		exit(-1);								// idk about exit statements
	}

	words = (char **) malloc(capacity * sizeof(char *));
	counts = (int *) malloc(capacity * sizeof(int));

	pthread_t threads[N];

	for (int i = 0; i < N; i++)
		pthread_create(&threads[i], NULL, worker, (void *) argv[i + 4]);
	for (int i = 0; i < N; i++)
		pthread_join(threads[i], NULL);

	// Sorting in descending order
	for (int i=0; i<num_words - 1; ++i)
	{
		for (int j=i+1; j<num_words; ++j)
		{
			int azResult = strcmp(words[j], words[i]);
			if ( (counts[j] > counts[i]) || ((counts[j] == counts[i]) && (azResult < 0)))
			{
				int temp_count = counts[i];
				counts[i] = counts[j];
				counts[j] = temp_count;

				char temp_word[ MAX_WORD_SIZE ];
				strcpy(temp_word, words[i]);
				strcpy(words[i], words[j]);
				strcpy(words[j], temp_word);
			}
		}
	}

	for (int i = 0; i < K && i < num_words; i++)
		fprintf(out_file, "%s %d\n", words[i], counts[i]);

	// Free the dynamically allocated arrays
	for (int i = 0; i < num_words; ++i)
	{
		free(words[i]);
	}
	free(words);
	free(counts);

	fclose(out_file);

	return 0;
} // main

void *worker(void *fileName)
{

	FILE *in_file = fopen(((char *)fileName), "r");		// read only

	// test for files not existing.
	if (in_file == NULL)
	{
		perror("Error!! Could not open file.");
		exit(-1);								// idk about exit statements
	}

	char word[ MAX_WORD_SIZE ];

	while (fscanf(in_file, "%s", word) == 1)			// &word ya da word emin deiglim + belki fscanf() yerine fgets()
	{
		// Convert the word to upper case
		for (int i=0; i<MAX_WORD_SIZE; ++i)
		{
			word[i] = toupper(word[i]);
		}

		// Check if the word already exists in the array of unique words.
		bool isFound = false;
		// pthread_mutex_lock(&lock);

		for (int i = 0; i < num_words; i++)
		{
			if (strcmp(word, words[i]) == 0)
			{
				counts[i]++;
				isFound = true;
				break;
			}
		}

		// If the word does not exist, add it to the array of unique words.
		if (!isFound)
		{
			// Check if the arrays need to be resized
			if (num_words >= capacity)
			{
				capacity *= 2;
				words = (char**) realloc(words, sizeof(char*) * capacity);
				counts = (int*) realloc(counts, sizeof(int) * capacity);
			}

			char *newWord = strdup(word);
			words[num_words] = newWord;
			++counts[num_words++];
		}

		// pthread_mutex_unlock(&lock);
	}

	fclose(in_file);

	return NULL;
} // worker
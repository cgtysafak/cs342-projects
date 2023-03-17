#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

void swap(int* xp, int* yp)
{
	int temp = *xp;
	*xp = *yp;
	*yp = temp;
}

int main(int argc, int *argv[])
{
	/*	argv[0] = ./proctopk
	 *	argv[1] = <K>
	 *	argv[2] = <output_file>
	 *	argv[3] = <input_file>
	 */

	const int MAX_WORD_SIZE = 64;

	int K = atoi(argv[1]);						// <K>
	FILE *out_file = fopen(argv[2], "w");		// write only
	FILE *in_file = fopen(argv[3], "r");		// read only

	// test for files not existing.
	if (in_file == NULL || out_file == NULL)
	{
		printf("Error! Could not open file.");
		exit(-1);
	}
	else if (K<=0)
	{
		printf("Error! K must be a positive integer.");
		exit(-1);
	}

	// write to file
	// fprintf(out_file, "this is a test %d\n", 31);

	// read from file
	// int no;
	// fscanf(in_file, "%d", &no);

	// Create an array to store unique words and their counts
	char words[ 500 ][ MAX_WORD_SIZE ];
	int wordCounts[ 500 ] = {0};
	int noOfWords = 0;
	char word[ MAX_WORD_SIZE ];

	while (fscanf(in_file, "%s", word) == 1)		// &word ya da word emin deiglim
	{
		// Convert the word to upper case
		for (int i=0; i<MAX_WORD_SIZE; ++i) {
			word[i] = toupper(word[i]);
		}

		// Check if the word already exists in the unique words array
		bool isFound = false;
		for (int i=0; i<noOfWords; ++i) {
			if (strcmp(words[i], word) == 0) {
				// If the word already exists, increment its count
				++wordCounts[i];
				isFound = true;

				break;
			}
		}

		// If the word does not exist, add it to the unique words array
		if (!isFound)
		{
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

	for (int i=0; i<K; ++i)
	{
		fprintf(out_file, "%s %d\n", words[i], wordCounts[i]);
	}

	return 0;
}
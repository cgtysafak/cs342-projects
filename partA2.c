#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/wait.h>

void processFile(int K, int fileIndex, char *fileName, int *word_array)
{
    const int MAX_WORD_SIZE = 64;

	FILE *in_file = fopen(fileName, "r");		// read only

	// test for files not existing.
	if (in_file == NULL)
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
	char words[ 50 ][ MAX_WORD_SIZE ];
	int wordCounts[ 50 ] = {0};
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
	
	//index ba;langi. noktasi her f'le icin
	int index = fileIndex * K;
	for (int i=0; i<K; ++i)
	{
	    word_array[i+index] = wordCounts[i];
		printf("%s %d%d\n", words[i], wordCounts[i], fileIndex);
	}


}

//int main(int argc, int *argv[])
int main()
{
	/*	argv[0] = ./proctopk
	 *	argv[1] = <K>
	 *	argv[2] = <output_file>
	 *	argv[3] = <input_file>
	 */
	 const int MAX_WORD_SIZE = 64;

	int K = 5;						// <K>
	//FILE *out_file = fopen(argv[2], "w");		// write only
	//FILE *in_file = fopen(argv[2], "r");		// read only

	 int array_size = K;
	 
	 int N = 2;

	 int *word_array = mmap(NULL, N * array_size * sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

	// initialize the shared array
    for (int i = 0; i < array_size*2; i++) {
        word_array[i] = 0;
    }
	for (int i = 0; i < N; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {

            processFile(K, i, "deneme.c", word_array);
		
	    printf("deneme.c");
            exit(1);
        }
        else {
        	wait(0);
        }
        
    }
    

	return 0;
}


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>

#define MAX_WORD_LEN 100

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc != 2) {
        if (rank == 0)
            printf("Usage: %s <input_file>\n", argv[0]);
        MPI_Finalize();
        return 1;
    }

    char* filename = argv[1];
    FILE* file = fopen(filename, "r");

    if (file == NULL) {
        if (rank == 0)
            printf("Error opening file: %s\n", filename);
        MPI_Finalize();
        return 1;
    }

    // Each process reads a portion of the file
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    long chunkSize = fileSize / size;
    long start = rank * chunkSize;
    long end = (rank == size - 1) ? fileSize : start + chunkSize;
    long chunkSizeWithMargin = end - start + MAX_WORD_LEN;

    char* chunk = (char*)malloc(chunkSizeWithMargin * sizeof(char));
    fseek(file, start, SEEK_SET);
    fread(chunk, sizeof(char), chunkSizeWithMargin, file);
    fclose(file);

    // Count words in the local chunk
    long wordCount = 0;
    char* word = strtok(chunk, " ");
    while (word != NULL) {
        wordCount++;
        word = strtok(NULL, " ");
    }

    // Reduce word count across all processes
    long totalWordCount;
    MPI_Reduce(&wordCount, &totalWordCount, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

    // Print word count from the root process
    if (rank == 0)
        printf("Total word count: %ld\n", totalWordCount);

    free(chunk);

    MPI_Finalize();
    return 0;
}

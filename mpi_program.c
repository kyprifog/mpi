
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define MAX_WORD_LENGTH 100

int main(int argc, char** argv) {
    int rank, size;
    char word[MAX_WORD_LENGTH];
    int localCount = 0;
    int globalCount = 0;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc < 2) {
        if (rank == 0) {
            printf("Usage: mpi_program <filename>\n");
        }
        MPI_Finalize();
        return 1; // Exit the program with a non-zero status to indicate an error
    }

    char* filename = argv[1];

    MPI_File file;
    MPI_Status status;

    // Open the file collectively
    MPI_File_open(MPI_COMM_WORLD, filename, MPI_MODE_RDONLY, MPI_INFO_NULL, &file);

    // Determine the file size
    MPI_Offset fileSize;
    MPI_File_get_size(file, &fileSize);

    // Calculate the chunk size and start/end offsets for each process
    MPI_Offset chunkSize = fileSize / size;
    MPI_Offset remaining = fileSize % size;
    MPI_Offset startOffset = rank * chunkSize;
    MPI_Offset endOffset = startOffset + chunkSize;

    // Adjust the end offset for the last process to account for the remaining portion
    if (rank == size - 1) {
        endOffset += remaining;
    }

    // Move the file pointer to the starting position of each process
    MPI_File_seek(file, startOffset, MPI_SEEK_SET);

    // Read the assigned portion of the file into a local buffer
    char* buffer = (char*)malloc((endOffset - startOffset + 1) * sizeof(char));
    MPI_File_read(file, buffer, endOffset - startOffset, MPI_CHAR, &status);
    buffer[endOffset - startOffset] = '\0'; // Null-terminate the buffer

    // Count words in the assigned portion of the file
    char* delimiters = " \t\n\r\f\v"; // Space, tab, newline, carriage return, form feed, vertical tab
    char* token = strtok(buffer, delimiters);
    while (token != NULL) {
        localCount++;
        token = strtok(NULL, delimiters);
    }

    free(buffer);
    MPI_File_close(&file);

    // Perform reduction to get partial word counts on each process
    int* counts = (int*)malloc(size * sizeof(int));
    MPI_Allgather(&localCount, 1, MPI_INT, counts, 1, MPI_INT, MPI_COMM_WORLD);

    // Compute the global word count by summing the partial counts
    for (int i = 0; i < size; i++) {
        globalCount += counts[i];
    }

    // Synchronize all processes before printing the final count
    MPI_Barrier(MPI_COMM_WORLD);

    // Print the final word count from the root process
    if (rank == 0) {
        printf("Total word count: %d\n", globalCount);
    }

    free(counts);
    MPI_Finalize();
    return 0;
}


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
    MPI_Offset startOffset = rank * chunkSize;
    MPI_Offset endOffset = startOffset + chunkSize;

    // Adjust the end offset for the last process to account for any remaining portion
    if (rank == size - 1) {
        endOffset = fileSize;
    }

    // Determine the size of the local buffer for each process
    MPI_Offset localBufferSize = endOffset - startOffset;
    char* localBuffer = (char*)malloc((localBufferSize + 1) * sizeof(char));

    // Set the shared file pointer to the start offset for each process
    MPI_File_set_view(file, startOffset, MPI_CHAR, MPI_CHAR, "native", MPI_INFO_NULL);

    // Read the local chunk of the file into the local buffer
    MPI_File_read(file, localBuffer, localBufferSize, MPI_CHAR, &status);
    localBuffer[localBufferSize] = '\0'; // Null-terminate the buffer

    // Count words in the local chunk of the file
    char* delimiters = " \t\n\r\f\v"; // Space, tab, newline, carriage return, form feed, vertical tab
    char* token = strtok(localBuffer, delimiters);
    while (token != NULL) {
        localCount++;
        token = strtok(NULL, delimiters);
    }

    free(localBuffer);
    MPI_File_close(&file);

    // Perform reduction to get partial word counts on each process
    MPI_Reduce(&localCount, &globalCount, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

    // Synchronize all processes before printing the final count
    MPI_Barrier(MPI_COMM_WORLD);

    // Print the final word count from the root process
    if (rank == 0) {
        printf("Total word count: %d\n", globalCount);
    }

    MPI_Finalize();
    return 0;
}


# Init

brew install open-mpi
mpicc mpi_program.c -o mpi_program
mpirun -np <num_processes> ./mpi_program text.txt

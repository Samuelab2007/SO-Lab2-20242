#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

int main(int argc, char *argv[]){

    if (argc < 2){    // se valida la cantidad de argumentos suministrados
        fprintf(stderr, "Error en la cantidad de argumentos pasados.\n");
        return 1;
    }

    struct timeval starting_time, ending_time; // variables de tiempo de incio y de fin
    pid_t pid = fork();                        // se crea un proceso

    if (pid < 0){                              
        perror("Error al intentar crear un proceso hijo.");
        return 1;
    }

    if (pid == 0){                            // proceso hijo

        execvp(argv[1], &argv[1]);
        perror("Exec fallo.");
        exit(1);

    } else {                                  // proceso padre

        gettimeofday(&starting_time, NULL);
        wait(NULL);
        gettimeofday(&ending_time, NULL);
        long seconds = ending_time.tv_sec - starting_time.tv_sec;
        long microseconds = ending_time.tv_usec - starting_time.tv_usec;
        double elapsed_time = seconds + microseconds * 1e-6;

        printf("Elapsed time:%.6f segundos\n", elapsed_time);

    }

    return 0;

}
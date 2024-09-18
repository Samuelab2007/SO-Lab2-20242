#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void prompt()
{
    // Get max pathlength for the current directory.
    long size = pathconf(".", _PC_PATH_MAX);
    // Create buffer to store the getcwd path.
    char current_directory_buffer[size];
    char *result = getcwd(current_directory_buffer, size);
    // If result == NULL, an error ocurred.
    if (result == NULL)
    {
        printf("Error obteniendo el cwd %i", errno);
    }
    else
    {
        // Change color to green
        printf("\033[0;32m");
        printf("wish:");
        // Change color to cyan
        printf("\033[0;36m");
        printf("%s> ", current_directory_buffer);
        // Reset color to default
        printf("\033[0m");
    }
}

int main(int argc, char *argv[])
{
    // The shell was called with more than one file
    if (argc > 2)
    {
        exit(1);
    }
    prompt();

    // Intento para leer una línea.
    char *buffer;
    size_t buffer_size = 10;
    getline(&buffer, &buffer_size, stdin);
    // int can_access = access("/bin/ls", F_OK);
    // printf("resultado de buscar el binario ls: %i\n", can_access);
}

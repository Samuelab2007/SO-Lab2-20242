/*
Integrantes:
- Samuel Acevedo Bustamante(1001016099)
- Manuela Gutierrez Cano(1037657256)
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <signal.h>

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

void read_shell_input()
{
    // Intento para leer una línea.
    char *buffer;
    size_t buffer_size = 1024;
    getline(&buffer, &buffer_size, stdin);
}

// Tokenize the parameters of the shell, returns an array with token pointers.
char **tokenize_entry(char *entry)
{

    // token_buffer es el token inicial
    static char token_buffer[1024];

    // Puntero hacia la ubicación de los token.(avanza a medida que se tokeniza)
    char *curr_str = NULL;
    // delim define los caracteres con los que se separa cada token.
    const char delim[3] = " ";

    // Array para guardar los punteros de los tokens.
    char **tokens = malloc(10 * sizeof(char *));
    int token_count = 0;

    snprintf(token_buffer, sizeof(token_buffer), "%s", entry);
    // Define token_ptr como el puntero hacia el token inicial
    char *token_ptr = token_buffer;

    // Llama strsep para tokenizar, guarda en curr_str el token obtenido
    curr_str = strsep(&token_ptr, delim);

    // Lo realiza en bucle y guarda en el array.
    while (curr_str != NULL)
    {
        if (curr_str[0] != '\0')
        {
            tokens[token_count] = curr_str;
            token_count++;
            curr_str = strsep(&token_ptr, delim);
        }
    }

    return tokens;
}
void execute_binary(const char *binary_name)
{

    pid_t pid = fork();
    // Primero se hace fork() a la consola. Y luego hacemos exec() en el proceso hijo
    // TODO: Hay que verificar que el comportamiento de los children no tengan problemas
    if (pid < 0)
    {
        // Fork failed
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0)
    {

        // Hay que utilizar la variable PATH
        const char *PATHS[] = {"/bin/", "/usr/bin/", NULL};

        // También se deben pasar argumentos a la función

        // Primero se verifica en las dos carpetas donde podrían estar los binarios

        char pathname[256];
        strcpy(pathname, PATHS[0]);
        strcat(pathname, binary_name);

        // Si no se tiene acceso a estas, se manda error.

        // Si se puede acceder a alguna de ellas. Se ejecuta

        int can_access = access(pathname, X_OK);

        if (can_access == 0)
        {
            char *args[] = {pathname, "-l", NULL};
            int code = execv(args[0], args);
        }
        else
        {
            printf("no se pudo encontrar el binario a ejecutar o no se tienen permisos de ejecución");
        }
    }
    printf("Started child process with PID: %d\n", pid);

    // Sleep father 1 second, to execute the command.
    sleep(1);

    // Los children solo se les puede mandar kill() y señales desde fuera. En este caso desde el father process
    if (kill(pid, SIGTERM) == 0)
    {
        printf("Sent sigterm to child process %d\n", pid);
    }
    else
    {
        perror("Failed to send SIGTERM");
    }
}

int main(int argc, char *argv[])
{
    // The shell was called with more than one file
    if (argc > 2)
    {
        exit(1);
    }

    while (1)
    {

        // 1. Display del prompt.
        prompt();

        // Se lee la entrada de la terminal.
        // read_shell_input();

        // Si muevo esto a la función read_shell_input() me da un segfault xd
        char *buffer;
        size_t buffer_size = 1024;
        getline(&buffer, &buffer_size, stdin);

        // buffer es donde queda guardado lo que se lee de la línea

        // 2. Tokenizar el comando ingresado en el shell.
        char **tokens = tokenize_entry(buffer);

        // TEMPORAL: Lee e imprime en pantalla el array de los tokens. Luego libera la memoria correspondiente a este.
        if (tokens != NULL)
        {
            for (int i = 0; tokens[i] != NULL; i++)
            {
                printf("Token %d: %s\n", i, tokens[i]);
            }
            free(tokens);
        }

        //  Mientras se pueda leer un token

        /* 3. Si el programa no es de los que tenemos que hacer la implementación propia. Lo ejecuto con execute_binary(), que
        también checkea su acceso.*/
        // execute_binary("ls");
    }
}

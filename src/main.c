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
#include <sys/wait.h>
#include <signal.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_PATHS 100

const char error_message[30] = "An error has occurred\n";

// Hay que utilizar la variable PATH

char *PATHS[MAX_PATHS];

char *PATHS[] = {"/bin/", "/usr/bin/", NULL};
int path_count = 3;

// Flag para indicar si hay que redirigir el comando, o si ya se encontró un ">"
int redirection = false;

void show_path()
{
    for (int i = 0; i < path_count; i++)
    {
        printf("Path: %s\n", PATHS[i]);
    }
}

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

char *trim_whitespace(char *str)
{
    char *end;

    // Trim leading space
    while (isspace((unsigned char)*str))
        str++;

    if (*str == 0) // All spaces?
        return str;

    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end))
        end--;

    // Write new null terminator character
    end[1] = '\0';

    return str;
}

int get_token_count(char **tokens)
{
    int length = 0;

    while (tokens[length] != NULL)
    {
        length++;
    }

    return length;
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
    char **tokens = malloc(50 * sizeof(char *));
    int token_count = 0;

    snprintf(token_buffer, sizeof(token_buffer), "%s", entry);
    // Define token_ptr como el puntero hacia el token inicial
    char *token_ptr = token_buffer;

    // Llama strsep para tokenizar, guarda en curr_str el token obtenido
    curr_str = strsep(&token_ptr, delim);

    curr_str = trim_whitespace(curr_str);

    // Lo realiza en bucle y guarda en el array.
    while (curr_str != NULL)
    {   
        if (curr_str[0] != '\0')
        {
            char *trimmed_token = trim_whitespace(curr_str);
            //char is_redirection_token = strcmp(trimmed_token, ">");

            if (strcmp(trimmed_token, ">") == 0)
            {
                // No me detecta cuando el simbolo ingresado es ">>"
                // write(STDOUT_FILENO, "Se encontro una redireccion", strlen("Se encontro una redireccion"));

                if (redirection)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(0);
                } else {
                    redirection = true;
                }
                
            } else if (strcmp(trimmed_token, ">>") == 0)
                {
                    write(STDOUT_FILENO, "Se encontro una redireccion", strlen("Se encontro una redireccion"));  
                    exit(1)
                }
                redirection = false;
                
            tokens[token_count] = trimmed_token;
            token_count++;
            curr_str = strsep(&token_ptr, delim);
        }
    }

    return tokens;
}

void execute_binary(char **tokens)
{

    /* Si hay una redirección, el símbolo ">" sería el antepenúltimo en los tokens.
    Ya que sería. token(n-2): ">", token(n-1): "output_file" token(n): NULL.
    Por lo que para verificar que no hayan varios archivos de salida, vamos a verificar que el antepenultimo token
    si cumpla esta condición. También sirve para verificar que si haya seteado un archivo de salida.
    */

    int token_count = get_token_count(tokens);
    int index_penultimo_token = token_count - 2;

    char *output_file = tokens[token_count - 1];
    if (redirection)
    {
        int comparacion_penultimo_token = strcmp(tokens[index_penultimo_token], ">");

        // Me está comparando esto sin ningún sendido. parece que lo hace aun cuando no hubo una redirección
        // Si la compraración se hace cuando no hay una redirección, los dos caracteres no son iguales, 
        // si es el caso de redirección, la comparación estaría en 0
        if (comparacion_penultimo_token == 0) // == en vez de !=
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        }

        // Subset del array de tokens
        // char **tokens_subset = malloc((token_count - 2) * sizeof(char *));

        // La parte de comandos de la entrada es hasta el token inmediatamente anterior a ">".
        for (int i = 0; i < index_penultimo_token; i++)
        {
            tokens[i] = tokens[i];
        }
        // Añade la terminación en NULL. Para no alterar las siguientes funciones.
        tokens[index_penultimo_token] = NULL;
        // Haciendo esto busco sólo pasar a execv la parte ejecutable del comando. Y el resto es hacia donde redirijo
    }

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
        // Primero se verifica en las dos carpetas donde podrían estar los binarios

        char pathname[256];
        const char *binary_name = tokens[0];

        strcpy(pathname, PATHS[0]);
        strcat(pathname, binary_name);

        // Si no se tiene acceso a estas, se manda error.

        if (access(pathname, R_OK) =! 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }

        if (access(binary_name, R_OK) =! 0)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
        

        // Si se puede acceder a alguna de ellas. Se ejecuta

        int can_access = access(pathname, X_OK);

        // El archivo se puede ejecutar.
        if (can_access == 0)
        {
            if (redirection)
            {
                FILE *fd_redirection = fopen(output_file, "w");

                int fileno_output = fileno(fd_redirection);

                // Asocia la salida y error estándar con el fd del archivo de redirección.
                dup2(fileno_output, STDERR_FILENO);
                dup2(fileno_output, STDOUT_FILENO);
                fclose(fd_redirection);
            }

            int arg_count = 0;
            while (tokens[arg_count] != NULL)
            {
                arg_count++;
            }

            // Allocate memory for args array
            char **args = malloc((arg_count + 1) * sizeof(char *));

            // Pass the contens of **tokens to args.
            for (int i = 0; i < arg_count; i++)
            {
                args[i] = tokens[i];
            }
            args[arg_count] = NULL; // Null terminate the args array

            int cmd_success = execv(pathname, args);

            free(args);
            if (cmd_success == -1)
            {
                exit(0);
            }
        }
        else
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
        }
    }

    // Wait for the child process to finish.
    int status;
    pid_t salida = waitpid(pid, &status, 0); // If waitpid is -1 an error occurred

    // If waitpid is -1 an error ocurred
    if (salida == -1)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    // Except to print messages about how the child process exited.
    if (WIFEXITED(status))
    {
        printf("Child process exited with status: %d\n", WEXITSTATUS(status));
    }
    else if (WIFSIGNALED(status))
    {
        printf("Child process was terminated by signal: %d\n", WTERMSIG(status));
    }
    
}

void execute_cd(char **tokens)
{
    // We already checked that it was a cd command.

    int token_count = 0;

    // If there is only one arg.(tokens[0] and nothing else) It is an error.

    for (int i = 0; tokens[i] != NULL; i++)
    {
        token_count++;
    }
    if ((token_count < 2) | (token_count > 2))
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
        exit(0);
    }
    else
    {
        int result = chdir(tokens[1]);
        if (result == -1)
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(1);
        }
    }

    // Otherwise call chdir() syscall.
}

// TODO: CORREGIR DESPUÉS
void change_path(char **tokens)
{

    int token_count = get_token_count(tokens);
    // printf("Token count: %i", token_count);

    for (int i = 0; i < path_count; i++)
    {
        PATHS[i] = "\0";
    }

    show_path();

    int path_index = 0;

    // Redefine PATHS variable.
    for (int j = 1; j < token_count + 1; j++)
    {
        PATHS[path_index] = tokens[j];
        path_index++;
    }
    show_path();
}

void execute_exit(char **tokens)
{
    int token_count = get_token_count(tokens);

    if (token_count > 1)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
    }
    exit(0);
}

// Checks for built-in commands. If none is found, returns -1.
int built_in_cmd(char **tokens)
{

    char *cmd_name = tokens[0];

    if (cmd_name != NULL)
    {
        // Check exit, cd, path.
        int is_exit = strcmp(cmd_name, "exit");
        int is_cd = strcmp(cmd_name, "cd");
        int is_path = strcmp(cmd_name, "path");

        if (is_exit == 0)
        {
            execute_exit(tokens);
        }
        else if (is_cd == 0)
        {
            execute_cd(tokens);
        }
        else if (is_path == 0)
        {
            change_path(tokens);
        }
        else
        {
            return -1;
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{

    int batch_mode = 0;
    // The shell was called with more than one file
    if (argc > 2)
    {
        exit(1);
    }

    if (argc == 2)
    {
        batch_mode = 1;
    }

    while (1)
    {
        // Define un buffer para leer los comandos de entrada(sea por tty o por batch).
        char *buffer;
        size_t buffer_size = 1024;
        redirection = false;
        // 1. Display del prompt. Batch mode desactivado.
        if (batch_mode == 0)
        {
            prompt();

            // Se lee la entrada de la terminal.
            getline(&buffer, &buffer_size, stdin);

            // 2. Tokenizar el comando ingresado en el shell.
            char **tokens = tokenize_entry(buffer);

            // 3. Ejecuta lo que lee por la entrada.
            if (tokens != NULL)
            {

                // Check for built-in commands.
                int found_builtin = built_in_cmd(tokens);

                if (found_builtin == -1)
                {
                    execute_binary(tokens);
                }

                free(tokens);
            }
        }
        // Batch mode. Lo mismo pero leyendo desde un archivo de entrada. El cual es pasado por argv[1]
        if (batch_mode == 1)
        {
            FILE *batch_fd = fopen(argv[1], "r");

            if (batch_fd == NULL)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
                exit(1);
            }
            else
            {
                while (getline(&buffer, &buffer_size, batch_fd) != -1)
                {
                    char **tokens = tokenize_entry(buffer);
                    if (tokens != NULL)
                    {
                        int found_builtin = built_in_cmd(tokens);

                        if (found_builtin == -1)
                        {
                            execute_binary(tokens);
                        }

                        free(tokens);
                    }
                }
            }
            fclose(batch_fd);
            return 0;
        }
    }
}

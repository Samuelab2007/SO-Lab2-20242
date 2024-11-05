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

const char error_message[30] = "An error has occurred\n";

// Hay que utilizar la variable PATH

char *path[10] = {"/bin/", NULL};

int path_count = 2;

// Flag para indicar si hay que redirigir el comando, o si ya se encontró un ">"
bool redirection = false;

// Flag para indicar si la ultima linea leída estaba vacía.
// Propenso a simplificar esta lógica con los continue y eso
bool last_line_was_empty = false;

void show_path()
{
    for (int i = 0; i < path_count; i++)
    {
        printf("Path[%i]: %s\n", i, path[i]);
    }
}

void print_commands(char **commands)
{

    int i = 0;

    while (commands[i] != NULL)
    {
        printf("Tokens[%i]: %s\n", i, commands[i]);
        i++;
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

char **find_parallel_commands(char *entry)
{
    static char token_buffer[1024];
    char *curr_str = NULL;
    const char separator[3] = "&";
    char **commands = malloc(50 * sizeof(char *));
    int parallel_count = 0;

    snprintf(token_buffer, sizeof(token_buffer), "%s", entry);

    char *token_ptr = token_buffer;
    token_ptr = trim_whitespace(token_ptr);

    curr_str = strsep(&token_ptr, separator);

    curr_str = trim_whitespace(curr_str);

    while (curr_str != NULL)
    {
        if (curr_str[0] != '\0')
        {
            char *trimmed_command = trim_whitespace(curr_str);

            commands[parallel_count] = trimmed_command;
            parallel_count++;

            curr_str = strsep(&token_ptr, separator);
        }
        else
        {
            last_line_was_empty = true;
            break;
        }
    }
    return commands;
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

    token_ptr = trim_whitespace(token_ptr);

    // Llama strsep para tokenizar, guarda en curr_str el token obtenido
    curr_str = strsep(&token_ptr, delim);

    curr_str = trim_whitespace(curr_str);
    // Lo realiza en bucle y guarda en el array.
    while (curr_str != NULL)
    {
        if (curr_str[0] != '\0')
        {
            char *trimmed_token = trim_whitespace(curr_str);

            // Compara para verificar los tokens especiales utilizados para indicar redirección y procesos paralelos
            int is_redirection_token = strcmp(trimmed_token, ">");

            if (is_redirection_token == 0)
            {

                if (redirection)
                {
                    write(STDERR_FILENO, error_message, strlen(error_message));
                    exit(0);
                }
                else
                {
                    redirection = true;
                }
            }

            tokens[token_count] = trimmed_token;
            token_count++;
            curr_str = strsep(&token_ptr, delim);
        }
        // La entrada es una linea en blanco
        else
        {
            last_line_was_empty = true;
            break;
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

    bool has_command = (index_penultimo_token != 0);

    char *output_file = tokens[token_count - 1];
    if (redirection)
    {
        int penultimo_separator_check = strcmp(tokens[index_penultimo_token], ">");
        if ((penultimo_separator_check != 0) || (!has_command))
        {
            write(STDERR_FILENO, error_message, strlen(error_message));
            exit(0);
        }

        for (int i = 0; i < index_penultimo_token; i++)
        {
            tokens[i] = tokens[i];
        }
        // Añade la terminación en NULL. Para no alterar las siguientes funciones.
        tokens[index_penultimo_token] = NULL;
        // Haciendo esto busco sólo pasar a execv la parte ejecutable del comando. Y el resto es hacia donde redirijo
    }

    // Acá se crean tantos procesos hijos como sean necesarios(uno por comando)
    pid_t pid = fork();
    // Primero se hace fork() a la consola. Y luego hacemos exec() en el proceso hijo
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

        strcpy(pathname, path[0]);
        strcat(pathname, binary_name);

        // Si no se tiene acceso a estas, se manda error.

        // Si se puede acceder a alguna de ellas. Se ejecuta
        int can_access = access(pathname, X_OK);

        FILE *fd_redirection;

        if (redirection)
        {
            fd_redirection = fopen(output_file, "w");
            if (fd_redirection == NULL)
            {
                perror("Failed to open output file");
                exit(EXIT_FAILURE);
            }

            int fileno_output = fileno(fd_redirection);
            if (fileno_output < 0)
            {
                perror("Failed to get file descriptor");
            }

            // Asocia la salida y error estándar con el fd del archivo de redirección.
            if (dup2(fileno_output, STDOUT_FILENO) < 0)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }

            if (dup2(fileno_output, STDERR_FILENO) < 0)
            {
                write(STDERR_FILENO, error_message, strlen(error_message));
            }
        }

        // El archivo se puede ejecutar.
        if (can_access == 0)
        {

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
        fclose(fd_redirection);
    }

    // Wait for the child process to finish.
    int status;
    waitpid(pid, &status, 0); // If waitpid is -1 an error occurred

    // Excerpt to print messages about how the child process exited.
    /*else
    {
        if (WIFEXITED(status))
        {
            printf("Child process exited with status: %d\n", WEXITSTATUS(status));
        }
        else if (WIFSIGNALED(status))
        {
            printf("Child process was terminated by signal: %d\n", WTERMSIG(status));
        }
    }*/
}

void execute_cd(char **tokens)
{

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
            exit(0);
        }
    }
}

// TODO: CORREGIR DESPUÉS
void change_path(char **tokens)
{

    int token_count = get_token_count(tokens);
    // printf("Token count: %i", token_count);

    // show_path();

    // Redefine path variable.
    for (int j = 0; j < token_count; j++)
    {
        // printf("TOken asignado como path: %s\n", tokens[j + 1]);
        path[j] = tokens[j + 1];
    }
    // show_path();
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
    last_line_was_empty = false;
    // The shell was called with more than one file
    if (argc > 2)
    {
        write(STDERR_FILENO, error_message, strlen(error_message));
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
            char **tokens;
            prompt();

            // Se lee la entrada de la terminal.
            getline(&buffer, &buffer_size, stdin);

            // Separo los distintos subcomandos de la entrada y luego realizo la tokenización de cada comando para ejecutarlo
            char **commands = find_parallel_commands(buffer);

            // print_commands(commands);

            // Aquí me encargo de separar los distintos comandos aún más
            for (int i = 0; commands[i] != NULL; i++)
            {
                tokens = tokenize_entry(commands[i]);
                // print_commands(tokens);
                //   3. Ejecuta lo que lee por la entrada.
                if (tokens != NULL)
                {
                    // Check for built-in commands.
                    int found_builtin = built_in_cmd(tokens);

                    if (found_builtin == -1)
                    {
                        execute_binary(tokens);
                    }
                }
            }
            free(tokens);
            // Si la ultima linea estaba vacía, reinicia el loop y continua funcionando.
            if (last_line_was_empty)
            {
                last_line_was_empty = false;
                continue;
            }

            // printf("Tokens first value: %s", tokens[0]);
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
                    char **tokens;
                    redirection = false;
                    // Separo los distintos subcomandos de la entrada y luego realizo la tokenización de cada comando para ejecutarlo
                    char **commands = find_parallel_commands(buffer);

                    // Aquí me encargo de separar los distintos comandos aún más
                    for (int i = 0; commands[i] != NULL; i++)
                    {
                        tokens = tokenize_entry(commands[i]);
                        // print_commands(tokens);
                        //   3. Ejecuta lo que lee por la entrada.
                        if (tokens != NULL)
                        {
                            // Check for built-in commands.
                            int found_builtin = built_in_cmd(tokens);

                            if (found_builtin == -1)
                            {
                                execute_binary(tokens);
                            }
                        }
                    }
                    free(tokens);

                    // Checkea, si la ultima linea estaba vacía. Se la salta y continua leyendo el batch file.
                    if (last_line_was_empty)
                    {
                        last_line_was_empty = false;
                        continue;
                    }
                }
            }
            fclose(batch_fd);
            return 0;
        }
    }
}

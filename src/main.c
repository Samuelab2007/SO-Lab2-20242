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

// Tokenize the parameters of the shell.
void tokenize_entry()
{

    // Tokenización de strings de entrada

    // token_buffer es el token inicial
    char token_buffer[50];

    // Puntero hacia la ubicación de los token.(avanza a medida que se tokeniza)
    char *curr_str = NULL;
    // delim define los caracteres con los que se separa cada token.
    const char delim[3] = " ";

    snprintf(token_buffer, 50, "%s", "string de pruebas");
    // Define token_ptr como el puntero hacia el token inicial
    char *token_ptr = token_buffer;

    // Llama strsep para tokenizar, guarda en curr_str el token obtenido
    curr_str = strsep(&token_ptr, delim);
    printf("%s\n", curr_str);
}
void execute_binary(const char *binary_name)
{
    // Primero se hace fork() a la consola. Y luego hacemos exec() en el proceso hijo

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
        char *args[] = {pathname, NULL};
        int code = execv(args[0], args);
    }
    else
    {
        printf("no se pudo encontrar el binario a ejecutar o no se tienen permisos de ejecución");
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

        // 2. Tokenizar el comando ingresado en el shell.
        // tokenize_entry();

        // Mientras se pueda leer un token

        // Creo que por esto se genera un seg fault.
        // Puede ser mejor opción utilizar un estilo de argc, ya que estos serían subproceos de alguna manera.
        /*while (curr_str != NULL)
        {
            curr_str = strsep(&token_ptr, delim);
            printf("%s\n", curr_str);
        }*/

        /* 3. Si el programa no es de los que tenemos que hacer la implementación propia. Lo ejecuto con execute_binary(), que
        también checkea su acceso.*/
        execute_binary("pwd");

        // Intento para leer una línea.
        char *buffer;
        size_t buffer_size = 10;
        getline(&buffer, &buffer_size, stdin);
    }
}

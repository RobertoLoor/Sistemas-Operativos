#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Error: Debe ingresar dos argumentos.\n");
        fprintf(stderr, "Modo correcto: %s <archivo_origen> <archivo_destino>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *originFile = argv[1];
    const char *destinyFile = argv[2];

    int origin = open(originFile, O_RDONLY);
    if (origin < 0) {
        perror("Error: Nombre del archivo Incorrecto:");
        printf("(%s)\n", originFile);
        exit(EXIT_FAILURE);
    }

    int destiny = open(destinyFile, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (destiny < 0) {
        perror("Error: No se ha podido crear el archivo de destino.\n");
        close(origin);
        exit(EXIT_FAILURE);
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Error al crear la tubería");
        close(origin);
        close(destiny);
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("Error al crear el proceso hijo");
        close(origin);
        close(destiny);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        close(pipe_fd[1]);
        char buffer[BUFFER_SIZE];
        ssize_t byteRead;
        while ((byteRead = read(pipe_fd[0], buffer, BUFFER_SIZE)) > 0) {
            if (write(destiny, buffer, byteRead) != byteRead) {
                perror("Error al escribir en el archivo de destino");
                close(destiny);
                close(pipe_fd[0]);
                exit(EXIT_FAILURE);
            }
        }
        close(destiny);
        close(pipe_fd[0]);
        exit(EXIT_SUCCESS);
    } else {
        close(pipe_fd[0]); 
        char buffer[BUFFER_SIZE];
        ssize_t byteRead;
        while ((byteRead = read(origin, buffer, BUFFER_SIZE)) > 0) {
            if (write(pipe_fd[1], buffer, byteRead) != byteRead) {
                perror("Error al escribir en la tubería");
                close(origin);
                close(pipe_fd[1]);
                exit(EXIT_FAILURE);
            }
        }
        close(origin);
        close(pipe_fd[1]);
        wait(NULL); 
    }

    return 0;
}

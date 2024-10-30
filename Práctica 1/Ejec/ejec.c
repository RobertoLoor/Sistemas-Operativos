#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <signal.h> 

void arbol() { 
    system("pstree");
}

int main(int argc, char *argv[]) { 
    if (argc != 2) {
        fprintf(stderr, "Error: Debe ingresar un argumento.\n");
        fprintf(stderr, "Modo correcto: %s <segundos>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int segundos = atoi(argv[1]);
    pid_t pidEjec, pidA, pidB, pidX, pidY, pidZ;

    pidEjec = getpid();
    printf("Soy el proceso ejec: mi pid es %d\n", pidEjec);

    pidA = fork();
    if (pidA == 0) { pidA = getpid();
        printf("Soy el proceso A: mi pid es %d. Mi padre es %d\n", pidA, pidEjec);

        pidB = fork();
        if (pidB == 0) { pidB = getpid();
            printf("Soy el proceso B: mi pid es %d. Mi padre es %d. Mi abuelo es %d\n", pidB, pidA, pidEjec);

            pidX = fork();
            if (pidX == 0) {
                printf("Soy el proceso X: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), pidB, pidA, pidEjec);
                sleep(segundos + 2);
                printf("Soy X (%d) y muero\n", getpid());
                exit(0);
            }
            
            pidY = fork();
            if (pidY == 0) {
                pidY = getpid();
                printf("Soy el proceso Y: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), pidB, pidA, pidEjec);
                sleep(segundos + 1);
                printf("Soy Y (%d) y muero\n", pidY);
                exit(0);
            }

            pidZ = fork();
            if (pidZ == 0) {
                printf("Soy el proceso Z: mi pid es %d. Mi padre es %d. Mi abuelo es %d. Mi bisabuelo es %d\n", getpid(), pidB, pidA, pidEjec);
                signal(SIGALRM, arbol);
                alarm(segundos);
                pause(); 
                printf("Soy Z (%d) y muero\n", getpid());
                exit(0);
            }

            wait(NULL);
            wait(NULL);
            wait(NULL);
            printf("Soy B (%d) y muero\n", pidB);
            exit(0);
        }

        wait(NULL);
        printf("Soy A (%d) y muero\n", getpid());
        exit(0);
    }

    wait(NULL);
    printf("Soy ejec (%d) y muero\n", pidEjec);

    return 0;
}
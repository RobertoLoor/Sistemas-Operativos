#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <sys/shm.h>
#include <sys/ipc.h>

int main(int argc, char *argv[]){
    if(argc != 3){
        fprintf(stderr, "Error: Debe ingresar dos argumentos.\n");
        fprintf(stderr, "Modo correcto: %s <x> <y>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int x = atoi(argv[1]), y = atoi(argv[2]);
    int *pids = NULL;
    int pid, i, j, k, cont, contHijos = x + 1;

    int shmid = shmget(IPC_PRIVATE, sizeof(int) * (x + y), IPC_CREAT | 0666);
    pids = (int *)shmat(shmid, 0, 0);
    pids[0] = getpid();

    for (i = 0; i < x; i++){
        pid = fork();
        if (pid  == 0){
            pids = (int *)shmat(shmid, 0, 0);
            pids[i + 1] = getpid();
        }
        else{ break;}
    }
    
    if (pid > 0){ wait(NULL);
        if (pids[0] == getpid()){
            printf("Soy el superpadre (%d): mis hijos finales son: ", getpid());
            for (j = 0; j < y; j++){ printf("%i ", pids[contHijos]);
                contHijos++;
            }
            printf("\n");
            shmdt((char *)pids);
            exit(0);
        }
    }

    if(pid == 0){
        cont = x + 1;
        for (j = 0; j < y; j++){ pid = fork();
            if (pid == 0){
                pids = (int *)shmat(shmid, 0, 0);
                pids[cont] = getpid();
                break;
            }
            cont++;
        }

        if (pid > 0){
            for (k = 0; k < j; k++){ wait(0);}
            shmdt((char *)pids);
            exit(0);
        }
        else{
            printf("Soy el subhijo %i: mis padres son: ", getpid());
            for(i = 0; i < x + 1; i++){printf("%i ", pids[i]);}
            printf("\n");
            shmdt((char *)pids);
            exit(0);
        }
    }

    shmctl(shmid, IPC_RMID, 0);
    
    return 0;
}


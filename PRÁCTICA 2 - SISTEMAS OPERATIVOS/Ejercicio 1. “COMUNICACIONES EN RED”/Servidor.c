#include <stdio.h> // Librería estándar de C. En el programa se utiliza para imprimir mensajes en la consola.
#include <stdlib.h> // Librería estándar de C. En el programa se utiliza para definir constantes y funciones de utilidad.
#include <string.h> // Librería estándar de C. En el programa se utiliza para manipular cadenas de texto.
#include <unistd.h> // Librería estándar de C. En el programa se utiliza para acceder a funciones de sistema.
#include <arpa/inet.h> // Librería para manejar direcciones IP. En el programa se utiliza para configurar la dirección del servidor.
#include <sys/socket.h> // Librería para manejar sockets. En el programa se utiliza para crear y conectar sockets.
#include <sys/wait.h> // Librería para manejar procesos. En el programa se utiliza para esperar a que los procesos hijos terminen.
#include <sys/ipc.h> // Librería para manejar claves IPC. En el programa se utiliza para generar claves únicas.
#include <sys/shm.h> // Librería para manejar memoria compartida. En el programa se utiliza para compartir datos entre procesos.
#include <sys/sem.h> // Librería para manejar semáforos. En el programa se utiliza para controlar el acceso a la memoria compartida.

#define PORT 9999 // Puerto en el que el servidor escuchará las conexiones.
#define BUFFER_SIZE 1024 // Tamaño del buffer para leer y escribir mensajes.
#define HISTORY_SIZE (BUFFER_SIZE * 10) // Tamaño del historial de comandos.

struct shared_data { // Estructura para almacenar los datos compartidos.
    char history[HISTORY_SIZE]; // Historial de comandos.
    int history_index; // Índice del historial.
};

void add_to_history(struct shared_data *shared_history, int semaphore, const char *command, int recognized) { // Función para agregar un comando al historial.
    struct sembuf sb = {0, -1, 0}; // Estructura para operaciones de semáforo. Se utiliza para bloquear el semáforo. El 0 indica el índice del semáforo, el -1 indica que se va a restar 1 al valor del semáforo y el 0 indica que se va a esperar a que el valor del semáforo sea 0.
    semop(semaphore, &sb, 1); // Bloquear el semáforo.

    snprintf(shared_history->history + shared_history->history_index, HISTORY_SIZE, "%s%s\n", // Agregar el comando al historial.
             command, recognized ? "" : " (No reconocido)"); // Si el comando fue reconocido, se agrega una cadena vacía. De lo contrario, se agrega "(No reconocido)".
    shared_history->history_index += strlen(command) + (recognized ? 1 : strlen(" (No reconocido)\n")); // Actualizar el índice del historial.

    sb.sem_op = 1; // Configurar la operación del semáforo para liberarlo. El 1 indica que se va a sumar 1 al valor del semáforo.
    semop(semaphore, &sb, 1); // Liberar el semáforo.
}

int execute_command(const char *command, char *result) { // Función para ejecutar un comando.
    int recognized = 1; // Indicador de si el comando fue reconocido. Se inicializa en 1.
    char test_command[BUFFER_SIZE]; // Buffer para almacenar el comando de prueba.
    snprintf(test_command, sizeof(test_command), "command -v %s > /dev/null 2>&1", command); // Crear el comando de prueba. Se redirige la salida estándar y de error a /dev/null para evitar que se muestre en la consola.
    if (system(test_command) != 0) { // Verificar si el comando existe.
        recognized = 0; // El comando no fue reconocido.
        snprintf(result, BUFFER_SIZE, "El comando '%s' no existe.\n", command); // Almacenar un mensaje de error en el buffer.
    } else { // El comando existe.
        FILE *fp = popen(command, "r"); // Ejecutar el comando y obtener un puntero al archivo de salida.
        if (fp != NULL) { // Verificar si se pudo ejecutar el comando.
            printf("Ejecutando comando: %s\n", command); // Imprimir un mensaje para indicar que se está ejecutando el comando.
            size_t total_read = 0; // Total de bytes leídos.
            while (fgets(result + total_read, BUFFER_SIZE - total_read, fp) != NULL) { // Leer la salida del comando.
                total_read = strlen(result); // Actualizar el total de bytes leídos.
                if (total_read >= BUFFER_SIZE - 1) { // Verificar si se ha leído todo el buffer.
                    break; // Salir del ciclo.
                } // Continuar leyendo la salida del comando.
            }
            pclose(fp); // Cerrar el archivo de salida.
            printf("Comando finalizado: %s\n", command); // Imprimir un mensaje para indicar que se ha finalizado el comando.
            printf("\n"); // Imprimir un salto de línea.
        } else { // No se pudo ejecutar el comando.
            recognized = 0; // El comando no fue reconocido.
            snprintf(result, BUFFER_SIZE, "Error al ejecutar el comando.\n"); // Almacenar un mensaje de error en el buffer.
        }
    }
    return recognized; // Retornar el indicador de si el comando fue reconocido.
}

int main() {
    int server_sock, client_sock; // Descriptores de socket para el servidor y el cliente. 
    struct sockaddr_in server_addr, client_addr; // Direcciones del servidor y del cliente.
    socklen_t client_addr_size; // Tamaño de la dirección del cliente.
    char buffer[BUFFER_SIZE]; // Buffer para leer y escribir mensajes.

    key_t key = ftok("historial", 65); // Generar una clave única para la memoria compartida y los semáforos.
    int shmid = shmget(key, sizeof(struct shared_data), 0666 | IPC_CREAT); // Crear un segmento de memoria compartida.
    struct shared_data *shared_history = (struct shared_data *) shmat(shmid, NULL, 0); // Asociar la memoria compartida a la estructura de datos.
    shared_history->history_index = 0; // Inicializar el índice del historial.

    int semaphore = semget(key, 1, 0666 | IPC_CREAT); // Crear un semáforo para controlar el acceso a la memoria compartida.
    semctl(semaphore, 0, SETVAL, 1); // Inicializar el valor del semáforo en 1.

    server_sock = socket(PF_INET, SOCK_STREAM, 0); // Crear un socket de tipo TCP (SOCK_STREAM).
    if (server_sock == -1) { // Verificar si se creó el socket.
        perror("Error al crear el socket"); // Imprimir un mensaje de error en caso de que no se haya creado el socket.
        exit(1); // Salir del programa en caso de error.
    }

    memset(&server_addr, 0, sizeof(server_addr)); // Limpiar la dirección del servidor.
    server_addr.sin_family = AF_INET; // Configurar la familia de direcciones (IPv4).
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // Configurar la dirección IP del servidor.
    server_addr.sin_port = htons(PORT); // Configurar el puerto del servidor.

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) { // Asociar el socket con la dirección del servidor.
        perror("Error en el bind"); // Imprimir un mensaje de error en caso de que no se haya podido asociar el socket. 
        exit(1); // Salir del programa en caso de error.
    }

    if (listen(server_sock, 5) == -1) { // Escuchar las conexiones entrantes.
        perror("Error en el listen"); // Imprimir un mensaje de error en caso de que no se haya podido escuchar las conexiones.
        exit(1); // Salir del programa en caso de error.
    }

    printf("Servidor escuchando en el puerto %d...\n", PORT); // Imprimir un mensaje para indicar que el servidor está escuchando en el puerto especificado.

    while (1) { // Ciclo para aceptar conexiones de clientes. Este ciclo se ejecuta de forma indefinida o hasta que se cierre el servidor.
        client_addr_size = sizeof(client_addr); // Obtener el tamaño de la dirección del cliente.
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_addr_size); // Aceptar la conexión de un cliente.
        if (client_sock == -1) { // Verificar si se aceptó la conexión.
            perror("Error en el accept"); // Imprimir un mensaje de error en caso de que no se haya aceptado la conexión.
            continue; // Continuar con el ciclo para aceptar conexiones.
        }

        printf("Cliente conectado.\n"); // Imprimir un mensaje para indicar que un cliente se ha conectado.

        while (1) { // Ciclo para recibir comandos del cliente. Este ciclo se ejecuta de forma indefinida o hasta que el cliente envíe el comando "FIN".
            memset(buffer, 0, BUFFER_SIZE); // Limpiar el buffer.
            int str_len = read(client_sock, buffer, BUFFER_SIZE - 1); // Leer el mensaje enviado por el cliente.
            if (str_len == -1) { // Verificar si se leyó el mensaje correctamente.
                perror("Error en la lectura del mensaje"); // Imprimir un mensaje de error en caso de que no se haya leído el mensaje.
                break; // Salir del ciclo para recibir comandos.
            }

            buffer[str_len] = '\0'; // Agregar el carácter nulo al final del mensaje.
            printf("Comando recibido: %s\n", buffer); // Imprimir el comando recibido.

            if (strcmp(buffer, "FIN") == 0) { // Verificar si el comando es "FIN".
                printf("Conexión finalizada por el cliente.\n"); // Imprimir un mensaje para indicar que el cliente ha finalizado la conexión.
                break; // Salir del ciclo para recibir comandos.
            }

            if (strcmp(buffer, "HISTORIA") == 0) { // Verificar si el comando es "HISTORIA".
                write(client_sock, shared_history->history, strlen(shared_history->history)); // Enviar el historial de comandos al cliente.
                continue; // Continuar con el ciclo para recibir comandos.
            }

            if (strcmp(buffer, "killServer") == 0) { // Verificar si el comando es "killServer".
                printf("¿Desea cerrar el servidor? (S/N): "); // Solicitar confirmación para cerrar el servidor.
                char respuesta[2]; // Buffer para almacenar la respuesta.
                fgets(respuesta, 2, stdin); // Leer la respuesta del usuario.
                if (respuesta[0] == 'S' || respuesta[0] == 's') { // Verificar si la respuesta es "S" o "s".
                    printf("Cerrando servidor...\n"); // Imprimir un mensaje para indicar que se está cerrando el servidor.
                    write(client_sock, "unuse", 5); // Enviar un mensaje al cliente para indicar que el servidor se cerrará.
                    close(client_sock); // Cerrar el socket del cliente.
                    close(server_sock); // Cerrar el socket del servidor.
                    shmdt((const void *) shared_history); // Desasociar la memoria compartida.
                    shmctl(shmid, IPC_RMID, NULL); // Eliminar el segmento de memoria compartida.
                    semctl(semaphore, 0, IPC_RMID); // Eliminar el semáforo.
                    exit(0); // Salir del programa.
                } else { // La respuesta no es "S" ni "s".
                    printf("Continuando con el servidor...\n"); // Imprimir un mensaje para indicar que se continuará con el servidor.
                    continue; // Continuar con el ciclo para recibir comandos.
                }
            }

            pid_t pid = fork(); // Crear un proceso hijo para ejecutar el comando.
            if (pid == 0) { // Verificar si se está en el proceso hijo.
                close(server_sock); // Cerrar el socket del servidor en el proceso hijo.

                int recognized; // Indicador de si el comando fue reconocido.
                char result[BUFFER_SIZE] = {0}; // Buffer para almacenar la respuesta del comando.

                recognized = execute_command(buffer, result); // Ejecutar el comando y obtener el indicador de si fue reconocido.

                add_to_history(shared_history, semaphore, buffer, recognized); // Agregar el comando al historial.

                write(client_sock, result, strlen(result)); // Enviar la respuesta del comando al cliente.

                close(client_sock); // Cerrar el socket del cliente en el proceso hijo.
                exit(0); // Salir del proceso hijo.
            } else if (pid < 0) { // Verificar si hubo un error al crear el proceso hijo.
                perror("Error en el fork"); // Imprimir un mensaje de error en caso de que no se haya podido crear el proceso hijo.
            }

            waitpid(pid, NULL, 0); // Esperar a que el proceso hijo termine.
        }

        close(client_sock); // Cerrar el socket del cliente.
    }

    shmdt((const void *) shared_history); // Desasociar la memoria compartida.
    shmctl(shmid, IPC_RMID, NULL); // Eliminar el segmento de memoria compartida.
    semctl(semaphore, 0, IPC_RMID); // Eliminar el semáforo.
    close(server_sock); // Cerrar el socket del servidor.
    return 0; // Devolver 0 para indicar que el programa finalizó correctamente.
}
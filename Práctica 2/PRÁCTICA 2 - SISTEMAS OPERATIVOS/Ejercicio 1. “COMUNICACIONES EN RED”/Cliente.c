#include <stdio.h> // Librería estándar de C. En el programa se utiliza para imprimir mensajes en la consola.
#include <stdlib.h> // Librería estándar de C. En el programa se utiliza para definir constantes y funciones de utilidad.
#include <string.h> // Librería estándar de C. En el programa se utiliza para manipular cadenas de texto.
#include <unistd.h> // Librería estándar de C. En el programa se utiliza para acceder a funciones de sistema.
#include <arpa/inet.h> // Librería para manejar direcciones IP.

#define PORT 9999 // Puerto en el que el servidor escuchará las conexiones.
#define BUFFER_SIZE 1024 // Tamaño del buffer para leer y escribir mensajes.

int main(int argc, char *argv[]) { // Función principal del programa. Se ejecuta al iniciar el programa.
    int sock; // Descriptor de socket
    struct sockaddr_in server_addr; // Dirección del servidor
    char buffer[BUFFER_SIZE]; // Buffer para leer y escribir mensajes
    char *ip; // Dirección IP del servidor

    if(argc == 2){ // Verificar si se proporcionó una dirección IP como argumento
        ip = argv[1]; // Dirección IP proporcionada como argumento
    }
    else{
        printf("Error en los argumentos. Debe proporcionar la dirección IP del servidor.\n"); // Imprimir un mensaje de error en caso de que no se haya proporcionado una dirección IP como argumento
        exit(1); // Salir del programa en caso de error
    }

    // Crear socket
    sock = socket(PF_INET, SOCK_STREAM, 0); // Crear un socket de tipo TCP (SOCK_STREAM)
    if (sock == -1) { // Verificar si se creó el socket
        perror("Error al crear el socket"); // Imprimir un mensaje de error en caso de que no se haya creado el socket
        exit(1); // Salir del programa en caso de error
    }

    // Configurar dirección del servidor
    memset(&server_addr, 0, sizeof(server_addr)); // Limpiar la dirección del servidor
    server_addr.sin_family = AF_INET; // Configurar la familia de direcciones (IPv4)
    server_addr.sin_addr.s_addr = inet_addr(ip); // Configurar la dirección IP del servidor
    server_addr.sin_port = htons(PORT); // Configurar el puerto del servidor

    // Conectar al servidor
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) { // Conectar al servidor
        perror("Error en la conexión"); // Imprimir un mensaje de error en caso de que no se haya podido conectar al servidor
        exit(1); // Salir del programa en caso de error
    }

    printf("Conectado al servidor.\n"); // Imprimir un mensaje para indicar que se ha conectado al servidor

    while (1) { // Ciclo para enviar comandos al servidor. Solo se sale del ciclo si se envía el comando "FIN".
        printf("Ingrese comando: \n(FIN para finalizar la conexión) \n(HISTORIA para ver el historial de comandos introducidos desde el inicio de la conexión): "); // Imprimir un mensaje para solicitar un comando al usuario. Además, se indica que se puede utilizar el comando "FIN" para finalizar la conexión y el comando "HISTORIA" para ver el historial de comandos.
        fgets(buffer, BUFFER_SIZE, stdin); // Leer el comando introducido por el usuario
        buffer[strcspn(buffer, "\n")] = 0; // Eliminar el salto de línea al final del comando. Esto se hace para evitar problemas al enviar el comando al servidor ya que se espera un comando sin saltos de línea.

        // Enviar comando al servidor
        write(sock, buffer, strlen(buffer)); // Enviar el comando al servidor
        
        // Salir si el comando es "FIN"
        if (strcmp(buffer, "FIN") == 0) { // Verificar si el comando es "FIN"
            printf("Finalizando conexión.\n"); // Imprimir un mensaje para indicar que se está finalizando la conexión
            break; // Salir del ciclo para enviar comandos
        }

        // Recibir respuesta del servidor
        int str_len = read(sock, buffer, BUFFER_SIZE - 1); // Leer la respuesta del servidor
        if (str_len == -1) { // Verificar si se leyó la respuesta
            perror("Error en la lectura de datos"); // Imprimir un mensaje de error en caso de que no se haya leído la respuesta
            break; // Salir del ciclo para enviar comandos
        }

        if(strcmp(buffer, "unuseerver") == 0){;
            memset(buffer, '-', BUFFER_SIZE); // Limpiar el buffer con guiones
            printf("\n\nEl servidor se ha cerrado. No se puede ejecutar ningún comando nuevo, a excepción de 'FIN'.\n\n");
        }
        
        buffer[str_len] = '\0'; // Agregar un carácter nulo al final de la respuesta ya que se espera una cadena de texto sin saltos de línea
        printf("Respuesta del servidor:\n%s\n", buffer); // Imprimir la respuesta del servidor
    }

    close(sock); // Cerrar el socket
    return 0; // Retornar un valor entero para indicar que el programa finalizó correctamente
}
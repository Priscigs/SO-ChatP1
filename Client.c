#include <vector>
#include <chrono>
#include <thread>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "chat.pb.h"
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/socket.h>

using namespace std;
using namespace chat;

void *getResponse(void* socket) {
    while (true) {
        char buffer[1024] = {0};
        // Recibe un mensaje del servidor
        int bytesReceived = recv(*(int*)socket, buffer, 1024, 0);
        if(bytesReceived == -1) {
            // Error al recibir el mensaje
            std::cout << "Error al intentar recibir un mensaje" << std::endl;
            pthread_exit(NULL);
        }
        if(bytesReceived == 0) {
            // La conexión se cerró
            std::cout << "La conexión se cerró" << std::endl;
            pthread_exit(NULL);
        }

        // Convierte el mensaje en una cadena de caracteres
        string request_str(buffer, bytesReceived);

        // Analiza la respuesta del servidor
        ServerResponse serverResponse;
        serverResponse.ParseFromString(request_str);
        if (serverResponse.code() == 200) {
            // La respuesta del servidor es correcta
            int option = serverResponse.option();
            switch (option) {
                case 1:
                    // Se registró un usuario
                    std::cout << "Se registró un usuario" << std::endl;
                    break;
                case 2:
                    // Listado de usuarios
                    if(serverResponse.has_connectedusers()) {
                        // Se muestra el número de usuarios conectados y su información
                        std::cout << "Usuarios: " << serverResponse.connectedusers().connectedusers_size() << std::endl;
                        for(int i = 0; i < serverResponse.connectedusers().connectedusers_size(); i++) {
                            User infoUser = serverResponse.connectedusers().connectedusers(i);
                            // Muestra información de cada usuario
                            showUserInfo(infoUser);
                        }
                    } else {
                        // Se muestra información de un usuario específico
                        User infoUser = serverResponse.userinforesponse();
                        User(infoUser);
                    }
                    break;
                case 3:
                    // Se cambió el estado de un usuario
                    std::cout << "Se cambió el estadp" << std::endl;
                    break;
                case 4:
                    if(serverResponse.has_message()) {
                        // Se muestra el mensaje entrante y su información
                        Message message = serverResponse.message();
                        std::cout << "Mensaje entrante de: " << message.sender() << ": " << message.message() << std::endl;
                    }
                    break;
                default:
                    // Error al recibir la opción deseada
                    std::cout << "Error al recibir la opción deseada." << std::endl;
                    break;
            }
            std::cout << std::endl;
        } else {
            // Error en la respuesta del servidor
            std::cout << "Error: " << "Sin respuesta del servidor." << std::endl;
        }
        memset(buffer, 0, 1024);
    }
}

void User(const UserInfo& infoUser) {
    std::cout << "Username: " << infoUser.user_name << std::endl;
    std::cout << "IP: " << infoUser.user_ip << std::endl;
    std::cout << "Estadp: " << getState(infoUser.user_state) << std::endl;
}

string getState(int status) {
    switch(status) {
        case 1:
            return "Activo";
        case 2:
            return "Ocupado";
        case 3:
            return "Inactivo";
        default:
            return "Desconocido";
    }
}

void options() {
    // Esperar entrada del usuario y enviar solicitudes al servidor
    string userInput;

    while(true) {
        std::cout << cliente << "@chat: ";

        // Leer entrada del usuario desde la consola
        getline(cin, userInput);

        // Determinar qué comando ingresó el usuario
        switch(getCommand(userInput)) {

            // Si el usuario ingresa "--ayuda", mostrar la ayuda
            case hash("--ayuda"):
                std::cout << "--ayuda: Te muestra una lista breve de los comandos a utilizar" << std::endl;
                std::cout << "--usuariosL: Se muestra la lista de los usuarios" << std::endl;
                std::cout << "--info [user_name]: Se muestra la información de los ususarios" << std::endl;
                std::cout << "--allM [message]: Se envía un mensaje a todos los usuarios conectados" << std::endl;
                std::cout << "--onlyM [user_name] [message]: Se envía un mensaje a un único ususarios" << std::endl;
                std::cout << "--infoMe: Nombre y estado" << std::endl;
                std::cout << "--estado [status_code]: Cambia tu estado a Activo, Inactivo u Ocupado" << std::endl;
                std::cout << "--salir: Salir del servidor" << std::endl;
                break;

            // Si el usuario ingresa "--usuariosL", solicitar una lista de usuarios al servidor
            case hash("--usuariosL"):
                // Se crea la solicitud de lista de usuarios
                UserOption listRequest;
                listRequest.set_option(2);
                UserList userInfoRequest;
                userInfoRequest.set_type_request(true);
                listRequest.set_allocated_inforequest(&userInfoRequest);

                // Se serializa la solicitud para enviarla al servidor
                string serializedRequest;
                listRequest.SerializeToString(&serializedRequest);
                int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);
                if(sendResult == -1) {
                    cerr << "No se pudo enviar el mensaje" << std::endl;
                    return 1;
                }

                // Se liberan los recursos de la solicitud
                listRequest.clear_option();
                listRequest.release_inforequest();
                break;

            // Si el usuario ingresa "--infoMe", solicitar información del usuario actual al servidor
            case hash("--infoMe"):
                // Se crea un objeto UserOption para almacenar la opción seleccionada por el usuario
                UserOption meRequest;
                // Se establece la opción correspondiente para solicitar información del usuario
                meRequest.set_option(2);
                // Se crea un objeto UserList para almacenar los detalles de la solicitud de información
                UserList meInfoRequest;
                // Se establece el tipo de solicitud como "false" para obtener información de un solo usuario
                meInfoRequest.set_type_request(false);
                // Se establece el nombre del usuario actual para obtener su información
                meInfoRequest.set_user(cliente);
                // Se establece el objeto meInfoRequest en el objeto meRequest
                meRequest.set_allocated_inforequest(&meInfoRequest);
                // Se convierte el objeto meRequest en una cadena de bytes
                string serializedRequest;
                meRequest.SerializeToString(&serializedRequest);
                // Se envía la solicitud al servidor
                int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);
                if(sendResult == -1) {
                    // Si la solicitud falla, se muestra un mensaje de error y se retorna 1
                    cerr << "No se pudo enviar el mensaje" << std::endl;
                    return 1;
                }
                // Se liberan los recursos utilizados por los objetos creados
                meRequest.clear_option();
                meRequest.release_inforequest();
                break;

            // Si el usuario ingresa "--info", solicitar información de otro usuario al servidor
            case hash("--info"):
                // Se le pide al usuario que ingrese el nombre del usuario del que quiere obtener información
                std::cout << "User name: ";
                string userName;
                getline(cin, userName);

                // Se crea una solicitud de información de usuario
                UserOption infoRequest;
                infoRequest.set_option(2);

                // Se crea un objeto UserInfoRequest para almacenar la información de la solicitud de información del usuario
                UserList infoRequestInfo;
                infoRequestInfo.set_type_request(false); // El tipo de solicitud es "false" para obtener información sobre un usuario específico
                infoRequestInfo.set_user(userName); // El nombre del usuario cuya información se está solicitando

                // Se asigna el objeto UserInfoRequest a la solicitud de usuario
                infoRequest.set_allocated_inforequest(&infoRequestInfo);

                // Se serializa la solicitud de usuario
                string serializedRequest;
                infoRequest.SerializeToString(&serializedRequest);

                // Se envía la solicitud de usuario al servidor
                int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);
                if(sendResult == -1) {
                    cerr << "No se pudo enviar el mensaje" << std::endl;
                    return 1;
                }

                // Se liberan los recursos de la solicitud de usuario
                infoRequest.clear_option();
                infoRequest.release_inforequest();
                break;

            // Permite al usuario enviar un mensaje a todos los usuarios conectados al servidor
            case hash("--allM"): 
                // Se solicita al usuario que escriba el mensaje a enviar a todos los usuarios.
                std::cout << "Mensaje: ";
                string message;
                getline(cin, message);
                
                // Se construye la solicitud con el mensaje a enviar.
                UserOption messageToAll;
                messageToAll.set_option(4);
                Message broadcastMessage;
                broadcastMessage.set_message(message);
                broadcastMessage.set_sender(cliente);
                broadcastMessage.set_message_type(true);
                messageToAll.set_allocated_message(&broadcastMessage);
                string serializedRequest;
                messageToAll.SerializeToString(&serializedRequest);
                        
                // Se envía la solicitud al servidor.
                int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);
                if(sendResult == -1) {
                    cerr << "No se pudo enviar el mensaje" << std::endl;
                    return 1;
                }
                
                // Se libera la memoria utilizada por la solicitud.
                messageToAll.clear_option();
                messageToAll.release_message();
                break;

            // Permite al usuario enviar un mensaje privado a un usuario específico
            case hash("--onlyM"): 
                // Solicitar al usuario que ingrese el destinatario y el mensaje
                std::cout << "Para: ";
                string to;
                getline(cin, to);
                std::cout << "Mensaje: ";
                string message;
                getline(cin, message);
                
                // Crear un objeto UserOption y configurar la opción como un mensaje privado
                UserOption messageToOne;
                messageToOne.set_option(4);
                
                // Crear un objeto Message y configurarlo con los detalles del mensaje privado
                Message privateMessage;
                privateMessage.set_message_type(false); // configurar como mensaje privado
                privateMessage.set_message(message);
                privateMessage.set_sender(cliente); // remitente es el usuario actual
                privateMessage.set_recipient(to); // destinatario es el usuario ingresado por el usuario
                
                // Asignar el objeto Message a la opción UserOption
                messageToOne.set_allocated_message(&privateMessage);
                string serializedRequest;
                messageToOne.SerializeToString(&serializedRequest);
                
                // Enviar el mensaje al servidor utilizando la función send()
                int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);
                if(sendResult == -1) {
                    cerr << "No se pudo enviar el mensaje" << std::endl;
                    return 1;
                }
                
                // Liberar los recursos asignados para evitar fugas de memoria
                messageToOne.clear_option();
                messageToOne.release_message();
                break;

            // Si el usuario ingresa "--estado", solicitar un cambio de estado al servidor
            case hash("--estado"):
                // Muestra un mensaje en la consola para solicitar al usuario que ingrese un nuevo estado.
                std::cout << "Nuevo estado: ";

                // Lee la entrada del usuario para el nuevo estado y lo guarda en la variable newStatusInput.
                string newStatusInput;
                getline(cin, newStatusInput);

                // Crea un objeto UserOption para enviar una solicitud de cambio de estado al servidor.
                UserOption changeStatusRequest;

                // Establece la opción en 3 para indicar que se está realizando una solicitud de cambio de estado.
                changeStatusRequest.set_option(3);

                // Crea un objeto Status y establece el nuevo estado ingresado por el usuario y el nombre de usuario actual.
                Status changeStatus;
                changeStatus.set_newstatus(stoi(newStatusInput));
                changeStatus.set_username(cliente);

                // Asigna el objeto Status recién creado a la solicitud de cambio de estado.
                changeStatusRequest.set_allocated_status(&changeStatus);

                // Serializa la solicitud de cambio de estado y la guarda en la variable serializedRequest.
                string serializedRequest;
                changeStatusRequest.SerializeToString(&serializedRequest);

                // Envía la solicitud serializada al servidor a través del socket y guarda el resultado en sendResult.
                int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);

                // Verifica si se pudo enviar la solicitud correctamente y muestra un mensaje de error si no es así.
                if(sendResult == -1) {
                    cerr << "No se pudo enviar el mensaje" << std::endl;
                    return 1;
                }

                // Limpia la opción y libera la memoria asignada al objeto Status.
                changeStatusRequest.clear_option();
                changeStatusRequest.release_status();

                // Sale del switch.
                break;

            default: {
                // Si el usuario ingresa un comando desconocido, se muestra un mensaje de error
                std::cout << "El comando es incorrecto. Puedes pedir ayuda con --ayuda" << std::endl;
                break;
            }
        }
    }
}

int main(int argc, char *argv[])
{
   // Verificar que se pasen los tres argumentos requeridos al programa
    if (argc != 4)
    {
        cout << "Debes ingresar el nombre del cliente, la IP del servidor y el puerto del servidor" << endl;
        return 1;
    }

    // Asignar los argumentos a variables legibles
    string cliente = argv[1];
    string ipServidor = argv[2];
    string puertoServidor = argv[3];

    // Imprimir información del cliente y del servidor
    std::cout << "Creación del cliente:" << std::endl;
    std::cout << "Nombre del cliente: " << cliente << std::endl;
    std::cout << "IP: " << ipServidor << std::endl;
    std::cout << "Puerto: " << stoi(puertoServidor) << std::endl;

    // Crear un socket para el cliente
    int socketClient = socket(AF_INET, SOCK_STREAM, 0);
    if (socketClient == -1)
    {
        cerr << "No se pudo crear el socket" << std::endl;
        return 1;
    }

    // Asignar la dirección del servidor al socket del cliente
    struct sockaddr_in serverAddress;
    serverAddress.sin_addr.s_addr = inet_addr(ipServidor.c_str());
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(stoi(puertoServidor));
    if (connect(socketClient, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) == -1)
    {
        cerr << "No se pudo conectar al servidor" << std::endl;
        return 1;
    }

    // Enviar una solicitud de registro al servidor
    UserOption registerRequest;
    NewUser registerUser;
    registerUser.set_username(cliente);
    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(serverAddress.sin_addr), clientIP, INET_ADDRSTRLEN);
    registerUser.set_ip(clientIP);
    registerRequest.set_option(1);
    registerRequest.set_allocated_newuser(&registerUser);
    string serializedRequest;
    registerRequest.SerializeToString(&serializedRequest);
    int sendResult = send(socketClient, serializedRequest.c_str(), serializedRequest.size(), 0);
    if (sendResult == -1)
    {
        cerr << "Error: No se pudo registrar un usuario" << std::endl;
        return 1;
    }
    registerRequest.clear_option();
    registerRequest.release_newuser();

    // Iniciar un hilo para recibir respuestas del servidor
    pthread_t id;
    pthread_create(&id, NULL, getResponse, (void *)&socketClient);

    options()
}
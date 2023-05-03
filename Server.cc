#include <pthread.h>
#include <cstdint>
#include <cstdbool>
#include <map>
#include <unistd.h>
#include <cstring>
#include <list>
#include <iostream>
#include <sys/socket.h>
#include "project.pb.h"
#include <netinet/in.h>

std::map<std::string, int> users_sockets;
std::map<std::string, int> users_state;
std::map<std::string, std::string> users;

#define PORT 8080


void* handle_client(void* user_socket) {
    // Instantiate the variable to store the connected user's name.
    std::string current_user = ""; 
    // Convert the socket to a manageable format by the system
    int socket = *((int *) user_socket);
    // Declare a variable that allows us to know if the server should keep listening to this specific
    // connection.
    bool user_while_flag = true;
    // The while loop allows us to keep track of any request from the referred connection.
    while(user_while_flag){

        // We decide to handle a 1024-byte buffer.
        char buffer[1024] = {0};

        // Write the information received through the socket into the buffer.
        int valread = recv(socket, buffer, 1024, 0);
        
        // If the amount of bytes received through the socket is greater than 0, it means that information was received,
        // so we proceed to handle the request.
        if (valread > 0) {
            // Convert the message received through the request in the socket to a string.
            std::string request_str(buffer, valread);

            // Instantiate the variable, based on the protocol, that will receive the information sent by the client
            // through the socket.
            chat::UserOption request;
            
            // Store the user's request in the request variable.
            if (!request.ParseFromString(request_str)) {
                std::cerr << "Error deserializing the request" << std::endl;

                        // Instantiate the variables we will be using in all the options.
                std::int32_t option = request.option();
                std::string user_name;
                bool user_flag = false;

                // Instantiate the variables we will be using in option 1.
                chat::NewUser newUser;
                std::string ip;
                
                // Instantiate the variables we will be using in option 2.
                bool info_user_type;
                chat::UserList user_info_request;
                chat::UsersOnline connected_users;
                chat::User user_info;

                // Instantiate the variables we will be using in option 3.
                chat::Status change_status;
                std::int32_t new_status;


                                // Instantiate the variables we will be using in option 4.
                bool m_type;
                std::string recipient;
                std::string sender;
                chat::Message message;
                std::string message_string;

                // Instantiate the variables we will be using for the server response.
                std::string response_str;
                chat::Answer server_response;
                std::string message_response;
                Copy code
                switch (option) {
                    case 1: {
                        // Extract variables from user request
                        newUser = request.newuser();
                        user_name = newUser.username();
                        ip = newUser.ip();

                        // Check if user is already registered or active
                        for (auto it = users.begin(); it != users.end(); ++it) {
                            if (it->first == user_name && users_state[user_name] != 3) {
                                // Notify client of error if user is already registered or active
                                message_response = "\n- User is already registered or active";
                                server_response.set_option(1);
                                server_response.set_code(400);
                                server_response.set_allocated_servermessage(&message_response);
                                user_flag = true;

                                if (!server_response.SerializeToString(&response_str)) {
                                    std::cerr << "Failed to serialize message." << std::endl;
                                }

                                send(socket, response_str.c_str(), response_str.length(), 0);
                                std::cout << "\n- The user that was sent is already registered or active: " << it->first << std::endl;
                            }
                        }

                        // If user is not registered or is inactive, create or activate the user and notify client
                        if (!user_flag || (user_flag && users_state[user_name] == 3)) {
                            users[user_name] = ip;
                            users_state[user_name] = 1;
                            users_sockets[user_name] = socket;
                            current_user = user_name;
                            message_response = "\n- User registered or activated successfully";
                            server_response.set_option(1);
                            server_response.set_code(200);
                            server_response.set_allocated_servermessage(&message_response);

                            if (!server_response.SerializeToString(&response_str)) {
                                std::cerr << "Failed to serialize message." << std::endl;
                            }

                            send(socket, response_str.c_str(), response_str.length(), 0);
                            std::cout << "\n- User registered successfully: " << user_name << std::endl;
                        }

                        // If no user is registered and another request is made, notify client of error
                        if (current_user == "") {
                            message_response = "\n- No user is connected";
                            server_response.set_option(1);
                            server_response.set_code(400);
                            server_response.set_allocated_servermessage(&message_response);

                            if (!server_response.SerializeToString(&response_str)) {
                                std::cerr << "Failed to serialize message." << std::endl;
                            }

                            send(socket, response_str.c_str(), response_str.length(), 0);
                            std::cout << "\n- No user associated with connection" << std::endl;
                        }

                        break;
                    }
                }


            }
        }
    }

   
}

int create_socket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        throw std::runtime_error("Error creating socket");
    }
    return sockfd;
}

void set_socket_options(int sockfd) {
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        throw std::runtime_error("Error setting socket options");
    }
}

void bind_socket(int sockfd) {
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    if (bind(sockfd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        throw std::runtime_error("Error binding socket to address");
    }
}

void listen_for_connections(int sockfd) {
    if (listen(sockfd, 3) < 0) {
        throw std::runtime_error("Error listening for incoming connections");
    }
}

void accept_connection(int sockfd) {
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int new_socket = accept(sockfd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
    if (new_socket < 0) {
        throw std::runtime_error("Error accepting incoming connection");
    }
    std::cout << "New connection accepted" << std::endl;
    pthread_t thread_id;
    if (pthread_create(&thread_id, NULL, handle_client, (void*)&new_socket) < 0) {
        std::cerr << "Error creating thread to handle client" << std::endl;
        close(new_socket);
    }
}

int main() {
    try {
        int sockfd = create_socket();
        set_socket_options(sockfd);
        bind_socket(sockfd);
        listen_for_connections(sockfd);
        std::cout << "Server started on port " << PORT << std::endl;
        while (true) {
            accept_connection(sockfd);
        }
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
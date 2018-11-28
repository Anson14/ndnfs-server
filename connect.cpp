//
// Created by anson on 18-11-26.
//
#include "connect.h"

using namespace std;

int init_socket(struct sockaddr_in &address, int backlog) {
    int server_fd;
    // sockaddr is decalred in <netinet/in.h>
    // this is a struct to indicate socket address
    // struct sockaddr_in address;
    int opt = 1;    // Haven't figure out what this argument indicate to
    char buffer[1024] = {0};

    // Creating socket file descriptor
    // Actualy, Socket is one type of special file
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // NOTE: This is really optional
    // Forcefully attaching socket to the port 8080
    // if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT,
    //                &opt, sizeof(opt)))
    // {
    //     perror("setsockopt");
    //     exit(EXIT_FAILURE);
    // }
    address.sin_family = AF_INET;   // Means IPV4
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *) &address,
             sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // NOTE: argument backlog = 3, it defines the maximum
    // length to which the queue of pending connection
    if (listen(server_fd, backlog) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    return server_fd;
}

int attr_to_json(vector<string> v) {
    Json::Value info;
    Json::StyledWriter sw;
    if (v.size() == 2) {
        server_getattribute((char *) v[1].c_str(), info);
        // Get attr sucess
        string attr_json = sw.write(info);
        send(new_socket, attr_json.c_str(), attr_json.size(), 0);
    } else {
        string error_json = sw.write(info);
        send(new_socket, error_json.c_str(), error_json.length(), 0);
    }
}
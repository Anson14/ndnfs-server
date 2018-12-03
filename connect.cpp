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

void senderror(Json::Value error) {
    Json::StyledWriter sw;
    string error_json = sw.write(error);
    send(new_socket, error_json.c_str(), error_json.length(), 0);
}

int attr_to_json(vector<string> v) {
    Json::Value info;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_getattribute(v[1].c_str(), info);
        // Get attr sucess
        string json_attr = sw.write(info);
        send(new_socket, json_attr.c_str(), json_attr.size(), 0);
    } else {
        senderror(info);
    }
}

int open_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 3) {
        Json::StyledWriter sw;
        server_open(v[1].c_str(), v[2].c_str(), root);
        string json_open = sw.write(root);
        send(new_socket, json_open.c_str(), json_open.size(), 0);
    } else {
        senderror(root);
    }
}

int read_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 4) {
        Json::StyledWriter sw;
        server_read(v[1].c_str(), stoi(v[2]), stoi(v[3]), root);
        string json_read = sw.write(root);
        send(new_socket, json_read.c_str(), json_read.size(), 0);
    } else {
        senderror(root);
    }
}

int write_to_json(vector<string> v) {
    Json::Value root;
    if (v.size() == 4) {
        Json::StyledWriter sw;
        char buffer[stoi(v[2]) +1];
        memset(buffer, '\0', sizeof buffer);
        read(new_socket, buffer, sizeof buffer);
        server_write(v[1].c_str(), buffer, stoi(v[2]), stoi(v[3]), root);
        string json_write = sw.write(root);
        send(new_socket, json_write.c_str(), json_write.size(), 0);
    } else {
        senderror(root);
    }
}

int release_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_release(v[1].c_str(), root);
        string json_release = sw.write(root);
        send(new_socket, json_release.c_str(), json_release.size(), 0);
    } else {
        senderror(root);
    }
}

int mknod_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_mknod(v[1].c_str(), root);
        string json_mknod = sw.write(root);
        send(new_socket, json_mknod.c_str(), json_mknod.size(), 0);
    } else {
        senderror(root);
    }
}

int rm_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_rm(v[1].c_str(), root);
        string json_rm = sw.write(root);
        send(new_socket, json_rm.c_str(), json_rm.size(), 0);
    } else {
        senderror(root);
    }
}

int readdir_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_readdir(v[1].c_str(), root);
        string json_readdir = sw.write(root);
        send(new_socket, json_readdir.c_str(), json_readdir.size(), 0);
    } else {
        senderror(root);
    }
}

int mkdir_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_mkdir(v[1].c_str(), root);
        string json_readdir = sw.write(root);
        send(new_socket, json_readdir.c_str(), json_readdir.size(), 0);
    } else {
        senderror(root);
    }
}

int rmdir_to_json(std::vector<std::string> v) {
    Json::Value root;
    if (v.size() == 2) {
        Json::StyledWriter sw;
        server_rmdir(v[1].c_str(), root);
        string json_rmdir = sw.write(root);
        send(new_socket, json_rmdir.c_str(), json_rmdir.size(), 0);
    } else {
        senderror(root);
    }
}
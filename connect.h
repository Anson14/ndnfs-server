//
// Created by anson on 18-11-26.
// Handling connection-related process
//

#ifndef NDNFS_SERVER_CONNECT_H
#define NDNFS_SERVER_CONNECT_H

#include "sys/socket.h"
#include "bits/stdc++.h"
#include "iostream"
#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <unistd.h>
#include "ndnfs-server.h"
#include "jsoncpp/json/json.h"

#define PORT 8080

int init_socket(struct sockaddr_in &address, int backlog);

void senderror(Json::Value error);

int attr_to_json(std::vector<std::string> v);

int open_to_json(std::vector<std::string> v);

int read_to_json(std::vector<std::string> v);

int write_to_json(std::vector<std::string> v);


#endif //NDNFS_SERVER_CONNECT_H

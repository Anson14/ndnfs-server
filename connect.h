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

#define PORT 8080

int init_socket(struct sockaddr_in &address, int backlog);


#endif //NDNFS_SERVER_CONNECT_H

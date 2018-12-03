//
// Created by anson on 18-12-3.
//
#ifndef NDNFS_SERVER_DIRECTORY_H
#define NDNFS_SERVER_DIRECTORY_H

#include "ndnfs-server.h"

int server_readdir(const char *path, Json::Value &root);

int server_mkdir(const char *path, Json::Value &root);

int server_rmdir(const char *path, Json::Value &root);

#endif //NDNFS_SERVER_DIRECTORY_H

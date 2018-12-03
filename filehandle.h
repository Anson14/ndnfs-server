//
// Created by anson on 18-11-27.
//

#ifndef NDNFS_SERVER_FILEHANDLE_H
#define NDNFS_SERVER_FILEHANDLE_H

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>
#include "sqlite3.h"
#include "ndnfs-server.h"
#include "filetype.h"
#include "mime-inference.h"

int server_getattribute(const char *name, Json::Value &root);

int ndnfs_updateattr(const char *path, int ver);

int server_open(const char *name, const char *mode, Json::Value &root);

int server_read(const char *path, size_t size, off_t offset, Json::Value &root);

int server_write(const char *path, const char *buf, size_t size, off_t offset, Json::Value &root);

int server_release(const char *path, Json::Value &root);

int server_mknod(const char *path, const char *mode, Json::Value &root);

int server_mknod(const char *path, Json::Value &root);

int server_rm(const char *path, Json::Value &root);

#endif //NDNFS_SERVER_FILEHANDLE_H

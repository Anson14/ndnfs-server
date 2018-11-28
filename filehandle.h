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

int server_getattribute(char * name, int jsonfile);

#endif //NDNFS_SERVER_FILEHANDLE_H

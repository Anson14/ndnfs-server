//
// Created by anson on 18-11-27.
//

#ifndef NDNFS_SERVER_NDNFS_SERVER_H
#define NDNFS_SERVER_NDNFS_SERVER_H

#include "connect.h"
#include "filehandle.h"
#include "bits/stdc++.h"
#include "logger.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include <assert.h>
#include <iostream>
#include <sstream>
#include <string>
#include <dirent.h>

#include <time.h>
#include <sys/time.h>
#include <utime.h>

#include <pthread.h>

extern const char *db_name;
extern sqlite3 *db;

namespace ndnfs {
//    extern ndn::Name certificateName;
//    extern ndn::ptr_lib::shared_ptr<ndn::KeyChain> keyChain;
    extern std::string global_prefix;
    extern std::string root_path;
    extern std::string logging_path;

    extern const int version_type;
    extern const int segment_type;
    extern const int seg_size;
    extern const int seg_size_shift;

    extern int user_id;
    extern int group_id;
}

enum orders{
    QUIT, SEND, DEFAULT, GETATTR
};

orders getOrder(char *order);

inline void SplitString(const std::string& s, std::vector<std::string>& v, const std::string& c)
{
    std::string::size_type pos1, pos2;
    pos2 = s.find(c);
    pos1 = 0;
    while(std::string::npos != pos2)
    {
        v.push_back(s.substr(pos1, pos2-pos1));

        pos1 = pos2 + c.size();
        pos2 = s.find(c, pos1);
    }
    if(pos1 != s.length())
        v.push_back(s.substr(pos1));
}



#endif //NDNFS_SERVER_NDNFS_SERVER_H

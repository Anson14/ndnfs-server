//
// Created by anson on 18-11-29.
//

#ifndef NDNFS_SERVER_FILESEGMENT_H
#define NDNFS_SERVER_FILESEGMENT_H

#include "ndnfs-server.h"
#include <ndn-cpp/data.hpp>
#include <ndn-cpp/common.hpp>
#include <ndn-cpp/security/security-exception.hpp>
#include "signature-states.h"

//inline int seek_segment(int doff)
//{
//    return (doff >> ndnfs::seg_size_shift);
//}
//
//inline int segment_to_size(int seg)
//{
//    return (seg << ndnfs::seg_size_shift);
//}

void copycurr_segment(const char* path, int cuur_ver);

int addtemp_segment(const char *path, const char *buf, size_t size, off_t offset);

int removetemp_segment(const char *path, int ver);

int sign_segment(const char *path, int ver, int seg, const char *data, int len);

int removenosign_segment(const char* path);

#endif //NDNFS_SERVER_FILESEGMENT_H

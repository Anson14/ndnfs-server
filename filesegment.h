//
// Created by anson on 18-11-29.
//

#ifndef NDNFS_SERVER_FILESEGMENT_H
#define NDNFS_SERVER_FILESEGMENT_H

#include "ndnfs-server.h"

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

#endif //NDNFS_SERVER_FILESEGMENT_H

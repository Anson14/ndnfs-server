#include "filesegment.h"

using namespace std;
using namespace ndn;

void copycurr_segment(const char *path, int cuur_ver) {
    FILE_LOG(LOG_DEBUG) << "copycurr_segment path=" << path << " current version=" << cuur_ver << endl;
    char temp_char[9] = ".segtemp";
    char path_temp[strlen(path) + strlen(temp_char) + 1];
    strcpy(path_temp, path);
    strcat(path_temp, temp_char);
    sqlite3_stmt *stmt;

    // get max seg
    sqlite3_prepare_v2(db, "SELECT MAX(segment) FROM file_segments WHERE path = ? AND version = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, cuur_ver);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        FILE_LOG(LOG_DEBUG) << "no segment exists" << endl;
        sqlite3_finalize(stmt);
        return;
    }
    int seg_max = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    for (int seg = 0; seg <= seg_max; ++seg) {
        sqlite3_prepare_v2(db,
                           "INSERT INTO file_segments (path, version, segment, signature, content) VALUES(?, 100000, ?, 'NONE', (SELECT content FROM file_segments WHERE (path = ? AND version = ? AND segment = ?)));",
                           -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, seg);
        sqlite3_bind_text(stmt, 3, path, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 4, cuur_ver);
        sqlite3_bind_int(stmt, 5, seg);
        res = sqlite3_step(stmt);
        if (res != SQLITE_DONE) {
            FILE_LOG(LOG_DEBUG) << "copy current segment error! path:" << path << " seg:" << seg << " cuur_ver:"
                                << cuur_ver << endl;
            sqlite3_finalize(stmt);
            return;
        } else FILE_LOG(LOG_DEBUG) << "copy current segment sucess!" << endl;
        sqlite3_finalize(stmt);
    }
}

//int addtemp_segment(const char *path, const char *buf, size_t size, off_t offset)
//{
//    FILE_LOG(LOG_DEBUG) << "addtemp_segment path=" << path<< " buf="<<buf<< " size="<< size<< " offset"<< offset  << endl;
//    sqlite3_stmt *stmt;
//    char buf_seg[ndnfs::seg_size];
//    int seg_size = ndnfs::seg_size;
//    int seg = offset / (seg_size); // current segment
//    int offset_saved = 0;
//    // Add ".segtemp" after path to indicate this is a temp version
//    char temp_char[9] = ".segtemp";
//    char path_temp[strlen(path) + strlen(temp_char) + 1];
//    strcpy(path_temp, path);
//    strcat(path_temp, temp_char);
//
//    // Insert temp segment into db
//    long long buf_size = (long long)size;
//    memset(buf_seg, '\0', seg_size);
//    if (offset % seg_size != 0)
//    {
//        // Get latest content we have inserted into db
//        sqlite3_prepare_v2(db, "SELECT content FROM file_segments WHERE path = ? AND segment =  ?;", -1, &stmt, 0);
//        sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
//        sqlite3_bind_int(stmt, 2, seg);
//        int res = sqlite3_step(stmt);
//
//        // TODO: Make offset work, now I just add buf behind th origianl string
//        if (res == SQLITE_ROW)
//        {
//            offset_saved = sqlite3_column_bytes(stmt, 0);
//            int seg_remain = seg_size - offset_saved;
//            memmove(buf_seg, (char *)sqlite3_column_blob(stmt, 0), offset_saved);
//            char content_add[seg_remain];
//            memcpy(content_add, buf, min((long long)seg_remain, buf_size));
////            strcat(buf_seg, content_add);
////          make offset useful
//            memcpy(buf_seg+(offset-(seg*seg_size)), content_add, sizeof content_add);
////          **********
//            sqlite3_finalize(stmt);
//            sqlite3_prepare_v2(db, "UPDATE file_segments SET content = ? WHERE path = ? AND segment = ? and version = 100000;", -1, &stmt, 0);
//            sqlite3_bind_blob(stmt, 1, buf_seg, sizeof content_add + sizeof buf_seg - (offset-(seg*seg_size)), SQLITE_STATIC);
//            sqlite3_bind_text(stmt, 2, path_temp, -1, SQLITE_STATIC);
//            sqlite3_bind_int(stmt, 3, seg);
//            res = sqlite3_step(stmt);
//            sqlite3_finalize(stmt);
//            offset_saved = seg_remain;
//            // FILE_LOG(LOG_DEBUG) << " Write first success\n";
//        }
//        else
//        {
//            sqlite3_finalize(stmt);
//        }
//    }
//
//    while ((buf_size - offset_saved) > 0) {
////         FILE_LOG(LOG_DEBUG) << "Wrinte a new segment seg="<< seg<< endl;
//
//        sqlite3_prepare_v2(db, "SELECT content FROM file_segments WHERE path = ? AND segment =  ?;", -1, &stmt, 0);
//        sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
//        sqlite3_bind_int(stmt, 2, seg);
//        int res = sqlite3_step(stmt);
//
//        // TODO: Make offset work, now I just add buf behind th origianl string
//        if (res == SQLITE_ROW) {
//            int originlen = sqlite3_column_bytes(stmt, 0);
//            memset(buf_seg, '\0', seg_size);
//            memcpy(buf_seg, (char *) sqlite3_column_blob(stmt, 0), originlen);
//            memcpy(buf_seg, buf + offset_saved, min((long long) seg_size, buf_size - offset_saved));
//            sqlite3_finalize(stmt);
//            sqlite3_prepare_v2(db,
//                               "INSERT OR REPLACE INTO file_segments (path,version,segment, signature, content) VALUES (?,100000,?,'NONE', ?);",
//                               -1, &stmt, 0);
//            sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
//            sqlite3_bind_int(stmt, 2, seg);
//            sqlite3_bind_blob(stmt, 3, buf_seg, min((long long) seg_size, max((buf_size - offset_saved),(long long) originlen)), SQLITE_STATIC);
//            int res = sqlite3_step(stmt);
//            sqlite3_finalize(stmt);
//            seg++;
//            offset_saved += min((long long) seg_size, buf_size - offset_saved);
//            if ((buf_size - offset_saved) < 0) {
//                break;
//            }
//        }
//    }
//}

int addtemp_segment(const char *path, const char *buf, size_t size, off_t offset) {
    FILE_LOG(LOG_DEBUG) << "addtemp_segment path=" << path << " buf=" << buf << " size=" << size << " offset" << offset
                        << endl;
    sqlite3_stmt *stmt;
    char buf_seg[ndnfs::seg_size];
    int seg_size = ndnfs::seg_size;
    int seg = offset / (seg_size); // current segment
    int seg_curr = 0;
    int size_curr = 0;
    int len_use = 0;
    // Add ".segtemp" after path to indicate this is a temp version
    char temp_char[9] = ".segtemp";
    char path_temp[strlen(path) + strlen(temp_char) + 1];
    strcpy(path_temp, path);
    strcat(path_temp, temp_char);
    // Get file current size
    sqlite3_prepare_v2(db,
                       "SELECT length(content), segment FROM file_segments WHERE path = ? AND segment = (SELECT MAX(segment) FROM file_segments WHERE path = ?);",
                       -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path_temp, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    int size_last = sqlite3_column_int(stmt, 0);
    seg_curr = sqlite3_column_int(stmt, 1);
    sqlite3_finalize(stmt);
    size_curr = seg_curr * seg_size + size_last;
    if (offset > size_curr) {
        FILE_LOG(LOG_ERROR) << "Offset is bigger than the file orignal size!" << endl;
        return -errno;
    }

    while (len_use < size) {
        if (seg <= seg_curr) {
            sqlite3_prepare_v2(db, "SELECT content FROM file_segments WHERE path = ? AND segment =  ?;", -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, seg);
            int res = sqlite3_step(stmt);
            if (res == SQLITE_ROW) {
                int originlen = sqlite3_column_bytes(stmt, 0);
                memset(buf_seg, '\0', seg_size);
                memcpy(buf_seg, (char *) sqlite3_column_blob(stmt, 0), originlen);
                int offset_remain = offset - (seg * seg_size);
                if (offset_remain < 0)
                    offset_remain = 0;
                int seg_use = min((int) (size - len_use), seg_size - offset_remain);
                memcpy(buf_seg + offset_remain, buf + len_use, seg_use);
                sqlite3_prepare_v2(db,
                                   "UPDATE file_segments SET content = ? WHERE path = ? AND segment = ? and version = 100000;",
                                   -1, &stmt, 0);
                sqlite3_bind_blob(stmt, 1, buf_seg, max(offset_remain + seg_use, originlen), SQLITE_STATIC);
                sqlite3_bind_text(stmt, 2, path_temp, -1, SQLITE_STATIC);
                sqlite3_bind_int(stmt, 3, seg);
                res = sqlite3_step(stmt);
                sqlite3_finalize(stmt);
                len_use += seg_use;
                seg++;
            } else {
                sqlite3_finalize(stmt);
            }
        } else {
            memset(buf_seg, '\0', seg_size);
            int seg_use = min(seg_size, (int) (size - len_use));
            memcpy(buf_seg, buf + len_use, seg_use);
            sqlite3_prepare_v2(db,
                               "INSERT OR REPLACE INTO file_segments (path,version,segment, signature, content) VALUES (?,100000,?,'NONE', ?);",
                               -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, seg);
            sqlite3_bind_blob(stmt, 3, buf_seg, seg_use,
                              SQLITE_STATIC);
            int res = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            len_use += seg_use;
            seg++;
        }
    }


}

int removetemp_segment(const char *path, int ver) {
    FILE_LOG(LOG_DEBUG) << "removetemp_segment path=" << path << endl;
    char temp_char[9] = ".segtemp";
    char path_temp[strlen(path) + strlen(temp_char) + 1];
    strcpy(path_temp, path);
    strcat(path_temp, temp_char);
    FILE_LOG(LOG_DEBUG) << "temp:" << path_temp << "ver;" << ver << endl;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "UPDATE file_segments SET path = ?, version = ? WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, ver);
    sqlite3_bind_text(stmt, 3, path_temp, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int sign_segment(const char *path, int ver, int seg, const char *data, int len) {
    FILE_LOG(LOG_DEBUG) << "sign_segment: path=" << path << std::dec << ", ver=" << ver << ", seg=" << seg << ", len="
                        << len << endl;

    string file_path(path);
    string full_name = ndnfs::global_prefix + file_path;
    // We want the Name(uri) constructor to split the path into components between "/", but we first need
    // to escape the characters in full_name which the Name(uri) constructor will unescape.  So, create a component
    // from the raw string and use its toEscapedString.

    string escapedString = Name::Component((uint8_t *) &full_name[0], full_name.size()).toEscapedString();
    // The "/" was escaped, so unescape.
    while (1) {
        size_t found = escapedString.find("%2F");
        if (found == string::npos)
            break;
        escapedString.replace(found, 3, "/");
    }
    Name seg_name(escapedString);

    seg_name.appendVersion(ver);
    seg_name.appendSegment(seg);
    FILE_LOG(LOG_DEBUG) << "sign_segment: segment name is " << seg_name.toUri() << endl;

    Data data0;
    data0.setName(seg_name);
    data0.setContent((const uint8_t *) data, len);
    // instead of putting the whole content object into sqlite, we put only the signature field.

    // FILE_LOG(LOG_DEBUG)<<"THIS IS GOING TO DETECT "<< seg_name<< "    "<< data<<endl;

    ndnfs::keyChain->sign(data0, ndnfs::certificateName);
    Blob signature = data0.getSignature()->getSignature();

    const char *sig_raw = (const char *) signature.buf();
    int sig_size = signature.size();

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
                       "INSERT OR REPLACE INTO file_segments (signature ,path, version, segment, content) VALUES (?, ?, ?, ?, ?);",
                       -1, &stmt, 0);
    // sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    // sqlite3_bind_int(stmt, 2, ver);
    // sqlite3_bind_int(stmt, 3, seg);
    sqlite3_bind_blob(stmt, 1, sig_raw, sig_size, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, ver);
    sqlite3_bind_int(stmt, 4, seg);
    sqlite3_bind_blob(stmt, 5, data, len, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_OK) {
        // FILE_LOG(LOG_DEBUG) << "WHY????" << endl;
    }
    sqlite3_finalize(stmt);

    // change ready_signed to ready;
    sqlite3_prepare_v2(db, "UPDATE file_system SET ready_signed = ? WHERE path = ? AND current_version = ? ;", -1,
                       &stmt, 0);
    enum SignatureState signatureState = READY;
    sqlite3_bind_int(stmt, 1, signatureState);
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, ver);
    res = sqlite3_step(stmt);
    return sig_size;
}

int removenosign_segment(const char *path) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "DELETE FROM file_segments WHERE path = ? AND signature = 'NONE';", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

void remove_file_entry(const char *path) {
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "DELETE FROM file_segments WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
//    if (res == SQLITE_OK)
//        FILE_LOG(LOG_DEBUG)<<"DELETE FS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n";
    sqlite3_finalize(stmt);


    sqlite3_prepare_v2(db, "DELETE FROM file_versions WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

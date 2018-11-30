//
// Created by anson on 18-11-27.
//
#include "filehandle.h"

using namespace std;

int server_getattribute(const char *name, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_getattribute: name=" << name << endl;
//    Json::Value root;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT mode, atime, current_version, size, nlink, type FROM file_system WHERE path = ?", -1,
                       &stmt, 0);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res == SQLITE_ROW) {
        root["issucess"] = 1;
        int type = sqlite3_column_int(stmt, 5);
        if (type == DIRECTORY) {
            root["st_mode"] = S_IFDIR | sqlite3_column_int(stmt, 0);
        } else if (type == REGULAR) {
            root["st_mode"] = S_IFREG | sqlite3_column_int(stmt, 0);
        } else
            return -errno;
        root["st_atime"] = sqlite3_column_int(stmt, 1);
        root["st_mtime"] = sqlite3_column_int(stmt, 2);
        root["st_size"] = sqlite3_column_int(stmt, 3);
        root["st_nlink"] = sqlite3_column_int(stmt, 4);
        root["st_uid"] = ndnfs::user_id;
        root["st_gid"] = ndnfs::group_id;
        sqlite3_finalize(stmt);

        // write json to file
        Json::StyledWriter sw;
        ofstream os;
        char jsondir[100];
        strcpy(jsondir, "/tmp/ndnfsjson");
        strcat(jsondir, name);
        strcat(jsondir, "_attr.json");
        os.open(jsondir);
        os << sw.write(root);
        os.close();

        return 0;
    } else {
        root["issucess"] = 0;
        sqlite3_finalize(stmt);
        FILE_LOG(LOG_ERROR) << "ndnfs_getattr: get_attr failed. path:" << name << ". Errno " << errno << endl;
        return -errno;
    }
}

int server_open(const char *name, const char *mode, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_open: path=" << name << " mode=" << mode << endl;

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT current_version FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        FILE_LOG(LOG_DEBUG) << "open error!" << endl;
        root["issucess"] = 0;
        sqlite3_finalize(stmt);
        return -ENOENT;
    }
    root["issucess"] = 1;

    int curr_ver = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    int temp_ver = time(0);

    string m = mode;
    if (m.find("w") != string::npos) {
        copycurr_segment(name, curr_ver);
//        FILE_LOG(LOG_DEBUG)<< "Here is a W"<< endl;
    }

    // When user open a file, make nlink+1
    sqlite3_prepare_v2(db, "UPDATE file_system set nlink = nlink + 1 WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, name, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    return 0;
}

int server_read(const char *path, size_t size, off_t offset, Json::Value & root) {
    FILE_LOG(LOG_DEBUG) << "server_read: path=" << path << ", offset=" << std::dec << offset << ", size=" << size
                        << endl;

    // First check if the file entry exists in the database,
    // this now presumes we don't want to do anything with older versions of the file
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT size FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        // File not exists
        root["issucess"] = 0;
        sqlite3_finalize(stmt);
        return -ENOENT;
    }
    if (sqlite3_column_int(stmt, 0) == 0) {
        // File is empty, So there is no segment in table file_segments
        sqlite3_finalize(stmt);
        return 0;
    }
    root["issucess"] = 1;
    sqlite3_finalize(stmt);

    // BIG CHANGE!
    // Read from  db now
    // test bt Anson at 2018.11.20
    char buf[size];
    int seg_size = ndnfs::seg_size;
    int seg = offset / seg_size;
    int len = 0;
    // Get the segment which nearst to offset
    sqlite3_prepare_v2(db,
                       "SELECT content FROM file_segments WHERE path = ? AND segment =  ? AND version = (SELECT current_version FROM file_system WHERE path = ?);",
                       -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, seg);
    sqlite3_bind_text(stmt, 3, path, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    if (res == SQLITE_ROW) {
        int content_size = sqlite3_column_bytes(stmt, 0);
        char *content[seg_size];
        memmove(content, (char *) sqlite3_column_blob(stmt, 0), content_size);
        int content_offset = offset - seg * seg_size;
        len += content_size - content_offset;
        memmove(buf, content + content_offset, len);
        // FILE_LOG(LOG_DEBUG)<< "content:"<< content_size<< endl;
        sqlite3_finalize(stmt);
        if (content_size < seg_size) {
            // means this segment is the last segment
            root["issucess"] = 1;
            root["readlen"] = len;
            root["buf"] = buf;
            return len;
        }
        // If the nearst segment is not enough (missing segment)
        while (len < size) {
            seg++;
            // FILE_LOG(LOG_DEBUG) << " len=" << len << " size=" << size << endl;
            sqlite3_prepare_v2(db,
                               "SELECT content FROM file_segments WHERE path = ? AND segment =  ? AND version = (SELECT current_version FROM file_system WHERE path = ?);",
                               -1, &stmt, 0);
            sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 2, seg);
            sqlite3_bind_text(stmt, 3, path, -1, SQLITE_STATIC);
            res = sqlite3_step(stmt);
            char *read_content[seg_size];
            if (res == SQLITE_ROW) {
                memset(content, '\0', seg_size);
                memset(read_content, '\0', seg_size);
                content_size = sqlite3_column_bytes(stmt, 0);
                // FILE_LOG(LOG_DEBUG)<< "circle: "<< content_size<< endl;
                memmove(content, (char *) sqlite3_column_blob(stmt, 0), content_size);
                int read_len = min(content_size, (int) size - len);
                memmove(read_content, content, read_len);
                sqlite3_finalize(stmt);
                // strcat(buf, read_content);
                // FILE_LOG(LOG_DEBUG)<< "circle: "<< read_len<< endl;
                memmove(buf + len, read_content, read_len);
                len += read_len;
                if (read_len < seg_size) {
                    root["readlen"] = len;
                    string read_buf = buf;
                    root["buf"] = read_buf;
                    return len;
                }
            } else {
                FILE_LOG(LOG_DEBUG) << "db error!!!" << endl;
                root["issucess"] = 0;
                sqlite3_finalize(stmt);
                // break;
                return -errno;
            }
        }
    }
}
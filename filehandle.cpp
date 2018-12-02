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

int ndnfs_updateattr(const char *path, int ver) {
    FILE_LOG(LOG_DEBUG) << "ndnfs_updateattr path:" << path << endl;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
                       "SELECT length(content), segment FROM file_segments WHERE path = ? AND version = ? AND segment = (SELECT MAX(segment) FROM file_segments WHERE path = ? AND version = ?);",
                       -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, ver);
    sqlite3_bind_text(stmt, 3, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 4, ver);
    // sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    int size = sqlite3_column_int(stmt, 0);
    int seg = sqlite3_column_int(stmt, 1);
    sqlite3_finalize(stmt);

    size += seg * ndnfs::seg_size;

    sqlite3_prepare_v2(db, "UPDATE file_system SET size = ? WHERE path = ?", -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, size);
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
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

int server_read(const char *path, size_t size, off_t offset, Json::Value &root) {
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
    memset(buf, '\0', sizeof buf);
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
        char content[seg_size];
        memset(content, '\0', sizeof content);
        memmove(content, (char *) sqlite3_column_blob(stmt, 0), content_size);
        int content_offset = offset - seg * seg_size;
        len += min(content_size - content_offset, (int) size);
        memset(buf, '\0', sizeof buf);
        strncpy(buf, content + content_offset, len);
        buf[len] = '\0';
//         FILE_LOG(LOG_DEBUG)<< "buf:"<< buf<< endl;
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

// TDDO: The offset is useless in this implementation while it should be useful
int server_write(const char *path, const char *buf, size_t size, off_t offset, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_write: path=" << path << std::dec << ", size=" << size << ", offset=" << offset
                        << endl;

    // First check if the entry exists in the database
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT current_version FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        root["issucess"] = 0;
        root["size"] = 0;
        sqlite3_finalize(stmt);
        return -ENOENT;
    }

    sqlite3_finalize(stmt);
    addtemp_segment(path, buf, size, offset);
    root["issucess"] = 1;
    root["size"] = (int) size;
    return size;
}

int server_release(const char *path, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_release: path=" << path << endl;
    int curr_version = time(0);
    int latest_version = 0;

    // First we check if the file exists
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT current_version FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        root["issucess"] = 0;
        sqlite3_finalize(stmt);
        return -ENOENT;
    }
    latest_version = sqlite3_column_int(stmt, 0);
    latest_version+=1;
    sqlite3_finalize(stmt);

//    if ((fi->flags & O_ACCMODE) != O_RDONLY) {

    // TODO: since older version is removed anyway, it makes sense to rely on system
    // function calls for multiple file accesses. Simplification of versioning method?
    //if (curr_ver != -1)
    //  remove_version (path, curr_ver);

    // remove temp version
    removetemp_segment(path, latest_version);

    sqlite3_prepare_v2(db, "UPDATE file_system SET current_version = ? WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_int(stmt, 1, curr_version); // set current_version to the current timestamp
    sqlite3_bind_text(stmt, 2, path, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    if (res != SQLITE_OK && res != SQLITE_DONE) {
        FILE_LOG(LOG_ERROR) << "ndnfs_release: update file_system error. " << res << endl;
        root["issucess"] = 0;
        return res;
    }
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "INSERT INTO file_versions (path, version) VALUES (?,?);", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, curr_version);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // After releasing, start a new signing thread for the file;
    // If a signing thread for the file in question has already started, kill that thread.
    int seg_all = 0;
    sqlite3_prepare_v2(db, "SELECT MAX(segment) FROM file_segments WHERE path = ? AND version =  ?", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, latest_version);
    res = sqlite3_step(stmt);
    seg_all = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);
    for (int seg = 0; seg <= seg_all; ++seg) {
        sqlite3_prepare_v2(db, "SELECT content FROM file_segments WHERE path = ? AND segment =  ?  AND version = ?",
                           -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, seg);
        sqlite3_bind_int(stmt, 3, latest_version);
        res = sqlite3_step(stmt);
        int len = 0;
        if (res == SQLITE_ROW) {
            len = sqlite3_column_bytes(stmt, 0);
            char data[len];
            memmove(data, (char *) sqlite3_column_blob(stmt, 0), len);
            sign_segment(path, curr_version, seg, data, len);
        }
        sqlite3_finalize(stmt);
        // FILE_LOG(LOG_DEBUG) << "release:::" << path << " " << curr_version << " " << seg << " " << len << endl;
    }

    ndnfs_updateattr(path, curr_version);
    // Delete segments that not been signed
    removenosign_segment(path);
    root["issucess"] = 1;
    // When user release a file, make nlink-1
    sqlite3_prepare_v2(db, "UPDATE file_system SET nlink = nlink-1 WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}
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
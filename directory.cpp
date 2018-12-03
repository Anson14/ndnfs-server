//
// Created by anson on 18-12-3.
//

#include "directory.h"

using namespace std;

int server_readdir(const char *path, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_readdir: path=" << path << endl;
    // read from db
    sqlite3_stmt *stmt;

    int level = 0; // director's level

    // Get father dir's level
    sqlite3_prepare_v2(db, "SELECT level FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        root["issucess"] = 0;
        return -ENOENT;
    }
    level = sqlite3_column_int(stmt, 0);
    level += 1;
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "SELECT path FROM file_system WHERE path LIKE ? AND level = ?;", -1, &stmt, 0);
    char path_notexact[100];
    strcpy(path_notexact, path);
    if (level == 1)
        strcat(path_notexact, "%");
    else
        strcat(path_notexact, "/%");
    sqlite3_bind_text(stmt, 1, path_notexact, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, level);
    // Means no such dir
    // if (res != SQLITE_ROW)
    Json::Value dir;
//    filler(buf, ".", NULL, 0);
//    filler(buf, "..", NULL, 0);
    dir[0] = ".";
    dir[1] = "..";
    int index_num = 2;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // FILE_LOG(LOG_DEBUG)<<"path: "<< sqlite3_column_text(stmt, 0)<< endl;
        char child_dir[sqlite3_column_bytes(stmt, 0)];
        strcpy(child_dir, (char *) sqlite3_column_text(stmt, 0));
        string prefix;
        string name;
        split_last_component(child_dir, prefix, name);
        // FILE_LOG(LOG_DEBUG)<<"path: "<< name<< endl;
//        filler(buf, name.c_str(), NULL, 0);
        dir[index_num] = name.c_str();
        index_num++;
    }

    root["dir"] = dir;
    root["issucess"] = 1;
    sqlite3_finalize(stmt);
    return 0;
}

int server_mkdir(const char *path, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_mkdir: path=" << path << endl;
    // cout<< "Step to  ndnfs_mkdir\n";

    string dir_path, dir_name;
    split_last_component(path, dir_path, dir_name);
    int level = 0;

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT * FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res == SQLITE_ROW) {
        // Cannot create file that has conflicting file name
        root["issucess"] = 0;
        sqlite3_finalize(stmt);
        return -ENOENT;
    }
    sqlite3_finalize(stmt);

    // Cannot create file without creationg necessary folders
    sqlite3_prepare_v2(db, "SELECT level FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, dir_path.c_str(), -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        sqlite3_finalize(stmt);
        return -ENOENT;
    }
    level = sqlite3_column_int(stmt, 0);
    level += 1;
    sqlite3_finalize(stmt);

    // Generate first version entry for the new file
    int ver = time(0);
    sqlite3_prepare_v2(db, "INSERT INTO file_versions (path, version) VALUES (?, ?);", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, ver);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Add the file(dir is a kind of file) entry to database
    // For directory, I use ready_signed to indicate the level
    // of which dir.
    sqlite3_prepare_v2(db,
                       "INSERT INTO file_system \
                      (path, current_version, mime_type, ready_signed, type, size, level) \
                      VALUES (?, ?, ?, ?, ?, 4096, ?);",
                       -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 2, ver); // current version
    char *mime_type = "";
    sqlite3_bind_text(stmt, 3, mime_type, -1, SQLITE_STATIC); // mime_type based on ext
    enum SignatureState signatureState = NOT_READY;
    sqlite3_bind_int(stmt, 4, signatureState);
    enum FileType fileType = DIRECTORY;
    sqlite3_bind_int(stmt, 5, fileType);
    sqlite3_bind_int(stmt, 6, level);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    root["issucess"] = 1;
    FILE_LOG(LOG_DEBUG) << "ndnfs_mkdir: Insert to database sucessful\n";
    return 0;
}

int server_rmdir(const char *path, Json::Value &root) {
    FILE_LOG(LOG_DEBUG) << "server_rmdir: path=" << path << endl;

    if (strcmp(path, "/") == 0) {
        root["issucess"] = 0;
        // Cannot remove root dir.
        return -EINVAL;
    }

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "select level FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    int res = sqlite3_step(stmt);
    if (res != SQLITE_ROW) {
        root["issucess"] = 0;
        sqlite3_finalize(stmt);
        FILE_LOG(LOG_DEBUG) << "rmdir error, no such directory!" << endl;
        return -errno;
    }
    sqlite3_finalize(stmt);

    char path_noexact[100];
    strcpy(path_noexact, path);
    strcat(path_noexact, "/%");

    // Delete file in this directory
    sqlite3_prepare_v2(db, "DELETE FROM file_system WHERE path LIKE ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path_noexact, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "DELETE FROM file_version WHERE path LIKE ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path_noexact, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "DELETE FROM file_segments WHERE path LIKE ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path_noexact, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    // Delete the directory
    sqlite3_prepare_v2(db, "DELETE FROM file_system WHERE path = ?;", -1, &stmt, 0);
    sqlite3_bind_text(stmt, 1, path, -1, SQLITE_STATIC);
    res = sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    root["issucess"] = 1;
    return 0;
}
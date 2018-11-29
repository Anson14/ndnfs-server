#include "filesegment.h"

using namespace std;

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

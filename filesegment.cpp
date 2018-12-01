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

int addtemp_segment(const char *path, const char *buf, size_t size, off_t offset)
{
    FILE_LOG(LOG_DEBUG) << "addtemp_segment path=" << path<< " buf="<<buf<< " size="<< size<< " offset"<< offset  << endl;
    sqlite3_stmt *stmt;
    char buf_seg[ndnfs::seg_size];
    int seg_size = ndnfs::seg_size;
    int seg = offset / (seg_size); // current segment
    int offset_saved = 0;
    // Add ".segtemp" after path to indicate this is a temp version
    char temp_char[9] = ".segtemp";
    char path_temp[strlen(path) + strlen(temp_char) + 1];
    strcpy(path_temp, path);
    strcat(path_temp, temp_char);

    // Insert temp segment into db
    long long buf_size = (long long)size;
    memset(buf_seg, '\0', seg_size);
    if (offset % seg_size != 0)
    {
        // Get latest content we have inserted into db
        sqlite3_prepare_v2(db, "SELECT content FROM file_segments WHERE path = ? AND segment =  ?;", -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, seg);
        int res = sqlite3_step(stmt);

        // TODO: Make offset work, now I just add buf behind th origianl string
        if (res == SQLITE_ROW)
        {
            offset_saved = sqlite3_column_bytes(stmt, 0);
            int seg_remain = seg_size - offset_saved;
            memmove(buf_seg, (char *)sqlite3_column_blob(stmt, 0), offset_saved);
            char content_add[seg_remain];
            memcpy(content_add, buf, min((long long)seg_remain, buf_size));
            strcat(buf_seg, content_add);
            sqlite3_finalize(stmt);
            sqlite3_prepare_v2(db, "UPDATE file_segments SET content = ? WHERE path = ? AND segment = ? and version = 100000;", -1, &stmt, 0);
            sqlite3_bind_blob(stmt, 1, buf_seg, offset_saved + min((long long)seg_remain, buf_size), SQLITE_STATIC);
            sqlite3_bind_text(stmt, 2, path_temp, -1, SQLITE_STATIC);
            sqlite3_bind_int(stmt, 3, seg);
            res = sqlite3_step(stmt);
            sqlite3_finalize(stmt);
            offset_saved = seg_remain;
            // FILE_LOG(LOG_DEBUG) << " Write first success\n";
        }
        else
        {
            sqlite3_finalize(stmt);
        }
    }

    while ((buf_size - offset_saved) > 0)
    {
//         FILE_LOG(LOG_DEBUG) << "Wrinte a new segment seg="<< seg<< endl;
        memset(buf_seg, '\0', seg_size);
        memcpy(buf_seg, buf + offset_saved, min((long long)seg_size, buf_size - offset_saved));
        sqlite3_prepare_v2(db, "INSERT OR REPLACE INTO file_segments (path,version,segment, signature, content) VALUES (?,100000,?,'NONE', ?);", -1, &stmt, 0);
        sqlite3_bind_text(stmt, 1, path_temp, -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, seg);
        sqlite3_bind_blob(stmt, 3, buf_seg, min((long long)seg_size, buf_size - offset_saved), SQLITE_STATIC);
        int res = sqlite3_step(stmt);
        sqlite3_finalize(stmt);
        seg++;
        offset_saved += min((long long)seg_size, buf_size - offset_saved);
        if ((buf_size - offset_saved) < 0)
        {
            break;
        }
    }
}

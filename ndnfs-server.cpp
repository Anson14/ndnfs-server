#include "ndnfs-server.h"

using namespace std;

const char *db_name = "/tmp/ndnfs.db";
sqlite3 *db;

string ndnfs::global_prefix = "/ndn/broadcast/ndnfs";
string ndnfs::root_path;
string ndnfs::logging_path = "";

const int ndnfs::seg_size = 8192; // size of the content in each content object segment counted in bytes
const int ndnfs::seg_size_shift = 13;

int ndnfs::user_id = 0;
int ndnfs::group_id = 0;

orders getOrder(char *order) {
    orders o = DEFAULT;
    if (strcmp(order, "quit") == 0)
        o = QUIT;
    else if (strcmp(order, "send") == 0)
        o = SEND;
    return o;
}

int main(int argc, char const *argv[]) {
    if (sqlite3_open(db_name, &db) == SQLITE_OK) {
        FILE_LOG(LOG_DEBUG) << "main: sqlite db open ok" << endl;
    } else {
        FILE_LOG(LOG_DEBUG) << "main: cannot connect to sqlite db, quit" << endl;
        sqlite3_close(db);
        return -1;
    }

    ndnfs::group_id = getgid();
    ndnfs::user_id = getuid();

    int server_fd, new_socket, valread;
    sockaddr_in address;
    server_fd = init_socket(address, 3);
    int addrlen = sizeof(address);

    if ((new_socket = accept(server_fd, (sockaddr *) &address,
                             (socklen_t *) &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    char buffer[1024];
    const char *hello = "Hello from server";
    valread = (int) read(new_socket, buffer, 1024);
    printf("%s\n", buffer);
    send(new_socket, hello, strlen(hello), 0);
    printf("Hello message sent\n");
//    while(true) {
    valread = (int) read(new_socket, buffer, 1024);
    vector<string> v;
    SplitString(buffer, v, " ");
    cout << v[0] << endl;
//        orders order = getOrder(buffer);

//        switch(order) {
//            case QUIT:
//                return 0;
//            case GETATTR:
//        }
//    }
}
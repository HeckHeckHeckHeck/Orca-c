#include <cstdlib>
#include <iostream>
#include <cstdio>
#include <cerrno>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pEp/pEpLog.hh>
#include <netdb.h>

#define SOKOL_IMPL
#include "sokol_time.h"
#undef SOKOL_IMPL


void init_sockaddr(sockaddr_in *name, std::string hostname, uint16_t port)
{

    hostent *hostinfo = nullptr;
    name->sin_family = AF_INET;
    name->sin_port = htons(port);
    hostinfo = gethostbyname(hostname.c_str());
    if (hostinfo == nullptr) {
        pEpLog("Unknown host: " + hostname);
        exit(1);
    }
    name->sin_addr = *(struct in_addr *)hostinfo->h_addr;
}


int make_socket(uint16_t port)
{
    int sock;
    sockaddr_in name;

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        pEpLog(strerror(errno));
        exit(1);
    }

    name.sin_family = AF_INET;
    name.sin_port = htons(port);
    name.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
        pEpLog(strerror(errno));
        exit(1);
    }

    return sock;
}

int main(int argc, char *argv[])
{
    pEp::Adapter::pEpLog::set_enabled(true);
    pEpLog("fdsfsd");
    int socket = make_socket(23232);

    return 0;
}
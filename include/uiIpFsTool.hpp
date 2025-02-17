#ifndef UIIPFSTOOL_HPP
#define UIIPFSTOOL_HPP

struct sockaddr_in;
bool readIpAddrFs(const char *path, sockaddr_in *addr);

#endif

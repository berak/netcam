#ifndef __birds_onboard__
#define __birds_onboard__

namespace Birds 
{
    int Server(int port);
    int Accept(int serv);

    int Client(const char *host, int port);

    int Write(int sock, const char *s, int len);

    char ReadByte(int sock);
    char * Read(int sock);

    char * Error();
    
    int Close(int sock);
}

#endif __birds_onboard__

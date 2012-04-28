//
//  on win32 this will need to link against ws2_32.lib
//


#include <string.h>
#include <stdio.h>
#include "birds.h"


#ifdef _WIN32  
    #include <winsock.h>
    #include <windows.h>
    #include <time.h>
    #define PORT	    unsigned long
    #define ADDRPOINTER   int*
    struct _INIT_W32DATA
    {
	   WSADATA w;
	   _INIT_W32DATA() {	WSAStartup( MAKEWORD( 2, 1 ), &w ); }
    } _init_once;
#else		/* ! win32 */
    #include <unistd.h>
    #include <sys/time.h>
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netdb.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #define PORT	    unsigned short
    #define SOCKET	  int
    #define HOSTENT	 struct hostent
    #define SOCKADDR	struct sockaddr
    #define SOCKADDR_IN  struct sockaddr_in
    #define ADDRPOINTER  unsigned int*
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR   -1
#endif /* _WIN32 */



namespace Birds 
{
    static char _errbuf[2012];

    int Client(const char *host, int port) 
    {
        _errbuf[0]=0;
        SOCKADDR_IN   address;
        SOCKET	   me = ::socket (AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
        if ( ( me ) == INVALID_SOCKET )
        {
            sprintf(_errbuf, "%s() error : couldn't create socket !", __FUNCTION__ );
            return -1;
        }

        unsigned long i_addr = ::inet_addr( host );
        if ( i_addr == INADDR_NONE ) {   // else : it was already an address
            HOSTENT *hostentry  = ::gethostbyname( host );
            if ( hostentry )
                i_addr =  *(unsigned long *)hostentry->h_addr_list[0];
        }		
        if ( i_addr == INADDR_NONE )
        {
            sprintf(_errbuf, "%s() error : couldn't resolve hostname '%s' !", __FUNCTION__, host );
            return -1;
        }

        address.sin_addr.s_addr = i_addr;
        address.sin_family      = AF_INET;
        address.sin_port        = ::htons(port);

        int res = ::connect( me, (SOCKADDR*) &address, sizeof (SOCKADDR_IN) );
        if ( res ) // connect returns 0 on success !
        {
            sprintf(_errbuf, "%s() error : couldn't connect to '%s:%d' !", __FUNCTION__,host , port);
            return -1;
        }
        return me;
    }

    //
    // reading will block.
    // yes, better leave the concurrency/threading stuff to rust ;)
    //
    char * Read( int sock ) 
    {
        static char buffer[8012];
        memset(buffer, 0, 8012);
        int res = ::recv( sock, buffer, 8012, 0 );
        if ( res > 0 ) 
        {
            buffer[res] = 0;
        }
        else 
        {
            sprintf(_errbuf, "%s() error : could not read from connection: %d %d !", __FUNCTION__,res, sock);
        }
        return buffer;
    }

    char  ReadByte( int sock ) 
    {
        char b = 0;
        int res = ::recv( sock, &b, 1, 0 );
        if ( res < 1 ) 
        {
            sprintf(_errbuf, "%s() error : could not read byte from connection: %d %d !", __FUNCTION__,res, sock);
        }
        return b;
    }

    int Write( int sock, const char *s, int len ) 
    { 
        if ( len < 1 ) { len = strlen(s); }
        return ::send( sock, s, len, 0 );
    }

    int Close( int sock ) 
    { 
        return::shutdown( sock, 2 );
    }

    char * Error() 
    {
        return _errbuf;
    }


    int Server(int port) 
    {
        _errbuf[0]=0;
        
        SOCKET      sock        = ::socket (AF_INET, SOCK_STREAM, IPPROTO_TCP) ;
        SOCKADDR_IN address;	   
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_family      = AF_INET;
        address.sin_port        = ::htons(port);

        if ( ::bind( sock, (SOCKADDR*) &address, sizeof(SOCKADDR_IN) ) == SOCKET_ERROR )
        {
            sprintf( _errbuf, "%s() error : couldn't bind sock %x to port %d !", __FUNCTION__, sock, port);
            return -1;
        }

        if ( ::listen( sock, 10 ) == SOCKET_ERROR )
        {
            sprintf( _errbuf, "%s() error : couldn't listen on sock %x on port %d !", __FUNCTION__, sock, port);
            return -1;
        }
        return sock;
    }    

    //
    // Accept will block
    //
    int Accept( int sock ) 
    {
        int         addrlen = sizeof(SOCKADDR);
        SOCKADDR_IN address = {0};	   
        SOCKET      client  = ::accept( sock,  (SOCKADDR*)&address, &addrlen );
        if ( client == SOCKET_ERROR )
        {
            sprintf( _errbuf, "%s() error : couldn't accept connection on sock %x !", __FUNCTION__, sock);
            return -1;
        }

        return client;
    }
}


//
// 8<------------------ snip demo/test code ----------------------
//  cl birds.cpp /D TEST_C ws2_32.lib   builds a simple ircclient
//  cl birds.cpp /D TEST_S ws2_32.lib   builds a micro-httpserver
//

//cl birds.cpp /D TEST_C ws2_32.lib
#ifdef TEST_C
// simple readonly irc client
int main( int argc, char **argv )
{
    int sock = Birds::Client("irc.freenode.net",6667);
    if ( sock==-1 ) return 1; // connection fail
    Birds::Write(sock, "NICK b1rd13\r\n", 0);
    Birds::Write(sock, "USER b1rd13 0 * :i am a bot\r\n", 0);
    Birds::Write(sock, "JOIN #pp1234\r\n", 0);
    //Birds::Write(sock, "PRIVMSG #pp1234 : hiho;) \r\n", 0);
    while(true) // sorry, readonly
    {
        char *m = Birds::Read(sock);
        if ( !strncmp(m, "PING",4) )
        {
            Birds::Write(sock, "PONG 12345 : hiho;) \r\n", 0);
        }
        printf("%s", m );
    }
    return 0;
}
#endif // TEST_C



//cl birds.cpp /D TEST_S ws2_32.lib
#ifdef TEST_S
// minimal httpserver on 4444
int main( int argc, char **argv )
{
    int serv = Birds::Server(4444);
	while ( serv > -1 ) {
        int c = Birds::Accept(serv);
        if (c == -1) { break; }
		char * h = Birds::Read(c);
        if (h) {
            char body[200];
            sprintf(body, "<B><I>;) [%d %d]</I></B>",serv,c);
            int l = strlen(body);
            char resp[400];
            sprintf(resp, "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nContent-Type: text/html\r\n\r\n%s",l,body);
            Birds::Write(c, resp, strlen(resp));
            printf(resp);
        }			
		Birds::Close(c);
    }
	Birds::Close(serv);
}
#endif

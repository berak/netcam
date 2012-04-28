#include "opencv/cv.h"
#include "opencv/highgui.h"
using namespace cv;

#include <conio.h>

#include "birds.h"
#include "thread.h"

#ifdef _WIN32
	using w32::Thread;
	struct CriticalSection : public CRITICAL_SECTION 
	{
		CriticalSection()	{ InitializeCriticalSection(this); 	}	
		~CriticalSection() 	{ DeleteCriticalSection (this);    	}

		void lock()			{ EnterCriticalSection( this );		}
		void unlock()		{ LeaveCriticalSection( this );		}
	} g_critter;
#else
	Mutex g_critter;
#endif




int count = 0;
int DELAY = 83; // 12 fps

class IpCapture : public Thread
{
	int sock;
	cv::Mat frame;
public:

	IpCapture()
		: sock(-1)
	{
	}

	IpCapture( const char * host,  const char * uri, int port=80 )
	{
		open(host,uri,port);
	}
	~IpCapture()
	{
		if ( sock > -1 )
			Birds::Close(sock);
	}
	bool isOpened()
	{
		return sock > -1;
	}
	bool open( const char * host,  const char * uri, int port=80 )
	{
		if ( isOpened() )
			Birds::Close(sock);

		sock = Birds::Client( host, port );
		if ( sock < 0 ) return false;

		char req[200];
		sprintf(req,"GET %s HTTP/1.1\r\nHOST:%s\r\n\r\n", uri, host );
		int r = Birds::Write(sock, req,0);
		Thread::start();
		return true;
	}
	const char *readline()
	{
		static char line[512];
		int n = 0;
		line[n] = 0;
		while( isOpened() && (n<512) )
		{
			char c = Birds::ReadByte(sock);
			if ( c == '\r' )
				continue;
			if ( c == '\n' )
				break;
			line[n] = c;
			n ++;
		}
		line[n] = 0;
		return line;
	}

	int readhead()
	{
		int nbytes = -1;
		int nlines = 0;
		while( isOpened() )
		{
			const char *l = readline();
			if ( nbytes < 0 )
				sscanf(l, "Content-Length: %i",&nbytes );
			if ( strlen(l) == 0  && nlines > 1 )
				break;
			nlines ++;
		}
		return nbytes;
	}

	virtual void run()
	{
		int frame_id = 0;
		readhead();
		while( isOpened() )
		{
			int nb = readhead();
			if ( nb < 1 ) nb = 0xffff;
			int finished = 0;
			int t = 0;
			std::vector<char> bytes;
			while ( true )
			{
				char c = Birds::ReadByte(sock);

				if ( c == char(0xff) ) 	finished = 1;
				else if ( c == char(0xd9) && finished == 1 ) 	break;
				else finished = 0;

				bytes.push_back(c);
				if ( (bytes.size() >= nb) )	break;

				const char * err = Birds::Error();
				if ( err && err[0] )
				{
					printf("%s\n", err );
					return;
				}
			}

			g_critter.lock();
				frame = imdecode(bytes,1);
			g_critter.unlock();

			printf( "%2d %6d\n", frame_id, bytes.size() );
			frame_id ++;
		}
	}

	friend IpCapture & operator >> ( IpCapture & cap, cv::Mat & mat );

};

IpCapture & operator >> ( IpCapture & cap, cv::Mat & mat );
IpCapture & operator >> ( IpCapture & cap, cv::Mat & mat )
{
	g_critter.lock();
		mat = cap.frame.clone();
	g_critter.unlock();
	return cap;
}




int main()
{
	Mat frame;
	IpCapture cap;
	bool ok = cap.open("traf4.murfreesborotn.gov","/axis-cgi/mjpg/video.cgi",80);
	//bool ok = cap.open("192.82.150.11","/mjpg/video.mjpg?camera=1",8081);
	if ( ! ok ) 
	{
		printf("no cam found ;(.\n");
		return 1;
	}

	namedWindow("video", 0);
	while ( cap.isOpened() )
	{
		cap >> frame;
		if ( ! frame.empty() )
			imshow("video",frame);

		if ( waitKey(100) > 0 )
			break;
	}

	return 0;
}



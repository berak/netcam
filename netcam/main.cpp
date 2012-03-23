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

class WebSession : public Thread
{
	int sock;
	cv::Mat & frame;
public:

	WebSession(int s, cv::Mat & frame) 
		: sock(s), frame(frame) 
	{
		 count ++;
		 printf("+ %2d " __FUNCTION__ "  %x\n",count, this);
	}

	~WebSession() 
	{
		count --;
		printf("- %2d " __FUNCTION__ "  %x\n",count, this);
	}

	virtual void run()
	{
		char * h = Birds::Read(sock); // we're not interested in the request, but flush the input anyway.
		printf(h);
		Birds::Write(sock,"200 HTTP/1.1 ok\r\n",0);
		Birds::Write(sock,
			"Server: Mozarella/2.2\r\n"
			"Accept-Range: bytes\r\n"
			"Connection: close\r\n"
			"Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\n"
			"\r\n",0);

		while ( frame.data ) 
		{
			g_critter.lock();
				// convert the image to JPEG
				std::vector<uchar>outbuf;
				std::vector<int> params;
				params.push_back(CV_IMWRITE_JPEG_QUALITY);
				params.push_back(100);
				cv::imencode(".jpg", frame, outbuf, params);
				int outlen = outbuf.size();
			g_critter.unlock();

			char head[400];
			sprintf(head,"--mjpegstream\r\nContent-Type: image/jpeg\r\nContent-Length: %lu\r\n\r\n",outlen);
			//sprintf(head,"--mjpegstream\r\nContent-Type: image/jpeg\r\n\r\n");
			Birds::Write(sock,head,0);
			int n2 = Birds::Write(sock,(char*)(&outbuf[0]),outlen);
			printf("%4x %d\n",sock,n2);
			if ( n2 < outlen )
				break;
			Sleep(DELAY);
		}
		Birds::Close(sock);
		delete this;
	}
};


class WebListener :  public Thread
{
	VideoCapture & cap;
	Mat & frame;
public:

	WebListener( VideoCapture &cap, Mat & frame )
		: cap(cap), frame(frame) {}

	virtual void run()
	{
		int serv = Birds::Server(7777);
		while(serv > -1) 
		{
			int cls = Birds::Accept(serv);
			if ( cls < 0 ) 
				break;
			if ( ! cap.isOpened() )
				break;

			WebSession * cl = new WebSession(cls,frame);
			cl->start();
		}
		Birds::Close(serv);
	}
};



int main()
{
	Mat frame;

	VideoCapture cap;
	bool ok = cap.open(0);
	if ( ! ok ) 
	{
		printf("no cam found ;(.\n");
		return 1;
	}

	WebListener *cl = new WebListener(cap,frame);
	cl->start();

	while ( cap.isOpened() )
	{
		g_critter.lock();
			cap >> frame;
		g_critter.unlock();

		if ( kbhit() )
			break;
		Sleep(DELAY);
	}

	delete cl;
	return 0;
}



#include "opencv/cv.h"
#include "opencv/highgui.h"
using namespace cv;

#include "birds.h"
#include "thread.h"


#ifdef _WIN32
	struct CriticalSection : public CRITICAL_SECTION 
	{
		CriticalSection()	{ InitializeCriticalSection(this); 	}	
		~CriticalSection() 	{ DeleteCriticalSection (this);    	}

		void lock()			{ EnterCriticalSection( this );		}
		void unlock()		{ LeaveCriticalSection( this );		}
	} g_critter;
#else
	#include <pthread.h>
	#include <unistd.h>
	class Mutex {
		pthread_mutex_t mutex;
	public:
		Mutex()              { pthread_mutex_init(&(mutex),NULL); }
		~Mutex()             { pthread_mutex_destroy(&(mutex)); }
		inline void lock()   { pthread_mutex_lock(&(mutex)); }
		inline void unlock() { pthread_mutex_unlock(&(mutex)); }
	} g_critter;
#endif



void client(int s, Mat & frame)
{
	Birds::Read(s); // we're not interested in the request, but flush the input anyway.
	Birds::Write(s,"200 HTTP/1.1 ok\r\n",0);
	Birds::Write(s,"Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\nPragma: no-cache\r\n\r\n",0);

	for ( ;; ) 
	{
		if(!frame.data) break;
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
		Birds::Write(s,head,0);
		int n2 = Birds::Write(s,(char*)(&outbuf[0]),outlen);
		if ( n2 < outlen )
			break;

		Sleep(50);
	}
	Birds::Close(s);
}


int count = 0;

class ClientThread : public w32::Thread
{
	int s;
	cv::Mat & frame;
public:
	ClientThread(int s, cv::Mat & frame) 
		: s(s), frame(frame) 
	{
		 count ++;
		 printf("+ %2d " __FUNCTION__ "\t%x\n",count, this);
	}
	~ClientThread() 
	{
		count --;
		printf("- %2d " __FUNCTION__ "\t%x\n",count, this);
	}
	virtual void run()
	{
		client(s, frame);
		delete this;
	}
};



class CaptureThread :  public w32::Thread
{
	VideoCapture & cap;
	Mat & frame;
public:
	CaptureThread(VideoCapture &cap, Mat & frame )
		: cap(cap), frame(frame) { }
	~CaptureThread() {}
	virtual void run()
	{
		bool ok = cap.open(0);
		while ( ok )
		{
			g_critter.lock();
				cap >> frame;
			g_critter.unlock();

			Sleep(30);
		}
		delete this;
	}
};



int main()
{
	VideoCapture cap;
	Mat frame;

	CaptureThread ct(cap,frame);
	ct.start();

	int serv = Birds::Server(7777);
	while(1) 
	{
		int c = Birds::Accept(serv);
		if ( c < 0 ) 
			break;

		ClientThread * cl = new ClientThread(c,frame);
		cl->start();
	}
	Birds::Close(serv);
	return 0;
}



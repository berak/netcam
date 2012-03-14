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
	Mutex g_critter;
#endif



void client(int sock, Mat & frame)
{
	Birds::Read(sock); // we're not interested in the request, but flush the input anyway.
	Birds::Write(sock,"200 HTTP/1.1 ok\r\n",0);
	Birds::Write(sock,"Content-Type: multipart/x-mixed-replace; boundary=mjpegstream\r\nPragma: no-cache\r\n\r\n",0);

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
		Birds::Write(sock,head,0);
		int n2 = Birds::Write(sock,(char*)(&outbuf[0]),outlen);
		if ( n2 < outlen )
			break;

		Sleep(50);
	}
	Birds::Close(sock);
}



int count = 0;

class ClientThread : public w32::Thread
{
	int sock;
	cv::Mat & frame;
public:
	ClientThread(int s, cv::Mat & frame) 
		: sock(s), frame(frame) 
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
		client(sock, frame);
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

	virtual void run()
	{
		for ( ;; )
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
	Mat frame;

	// start the capture in its own thread
	VideoCapture cap;
	bool ok = cap.open(0);
	if ( ! ok ) 
		return 1;
	CaptureThread ct(cap,frame);
	ct.start();

	// start the webserver in the main thread
	int serv = Birds::Server(7777);
	while(serv > -1) 
	{
		// one more thread per client
		int cls = Birds::Accept(serv);
		if ( cls < 0 ) 
			break;

		ClientThread * cl = new ClientThread(cls,frame);
		cl->start();
	}
	Birds::Close(serv);
	return 0;
}



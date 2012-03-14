#include "opencv/cv.h"
#include "opencv/highgui.h"
using namespace cv;

#include "birds.h"
#include "thread.h"


struct CriticalSection : public CRITICAL_SECTION 
{
	CriticalSection()	{ InitializeCriticalSection(this); 	}	
	~CriticalSection() 	{ DeleteCriticalSection (this);    	}

	void lock()			{ EnterCriticalSection( this );		}
	void unlock()		{ LeaveCriticalSection( this );		}
} g_critter;


void client(int s, Mat & frame)
{
	Birds::Read(s);
	//printf(Birds::Read(s));
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
		//printf("%4x %d %d\n",s,outlen,n2 );
		if ( n2 < outlen )
			break;

		Sleep(50);
	}
	Birds::Close(s);
}


int count = 0;
int main()
{
	VideoCapture cap;
	Mat frame;

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
	} * ct = new CaptureThread(cap,frame);
	ct->start();

	int serv = Birds::Server(7777);
	while(1) 
	{
		int c = Birds::Accept(serv);
		if ( c < 0 ) 
			break;

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
				Thread::stop();
				delete this;
			}
		} * cl = new ClientThread(c,frame);
		cl->start();
	}
	Birds::Close(serv);
	return 0;
}



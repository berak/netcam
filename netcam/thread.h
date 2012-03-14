#ifndef __ENGINE_THREAD_H__
#define __ENGINE_THREAD_H__


#ifdef _WIN32
 #include <windows.h>
 #include <process.h>

namespace w32
{
	///////////////////////////////////////////////////////////////////////////////////////
	// Mutex
	///////////////////////////////////////////////////////////////////////////////////////
	class Mutex {
		HANDLE mutex;
	public:
		Mutex()              { mutex = CreateMutex(NULL,FALSE,NULL); }
		~Mutex()             { CloseHandle(mutex); }
		inline void lock()   { WaitForSingleObject(mutex,INFINITE); }
		inline void unlock() { ReleaseMutex(mutex); }
	};
	
	
	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Thread
	///////////////////////////////////////////////////////////////////////////////////////
	
	class Thread {
	protected:
		HANDLE thread;
	public:
		Thread() : thread(0) {}
		virtual ~Thread() { stop(); }
		void start();
		void stop();
		void sleep( int millis );
		const bool running() { return (thread != 0); }
		virtual void run() = 0;
	};
	
	inline unsigned long WINAPI serve_thread( Thread *t ) {
		t->run(); // please exit by calling 't->stop()'
		return 0;
	}
	
	
	inline void Thread::start() {
		unsigned long id;
		thread = CreateThread( 0, 0,
			(LPTHREAD_START_ROUTINE)serve_thread,
			(void*)this,
			0,
			&id );
	}
	
	inline void Thread::stop() {
		if ( thread ) {
			//TerminateThread( thread, 0 );
			CloseHandle( thread );
			thread = 0;
		}
	}
	
	inline void Thread::sleep( int millis ) {
		::Sleep( millis );
	}
	
	
	
	#else // ! _WIN32, use pthreads:
	 #include <pthread.h>
	 #include <stdio.h>
	 #include <unistd.h>
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Mutex
	///////////////////////////////////////////////////////////////////////////////////////
	class Mutex {
		pthread_mutex_t mutex;
	public:
		Mutex()              { pthread_mutex_init(&(mutex),NULL); }
		~Mutex()             { pthread_mutex_destroy(&(mutex)); }
		inline void lock()   { pthread_mutex_lock(&(mutex)); }
		inline void unlock() { pthread_mutex_unlock(&(mutex)); }
	};
	
	
	
	///////////////////////////////////////////////////////////////////////////////////////
	// Thread
	///////////////////////////////////////////////////////////////////////////////////////
	class Thread {
	protected:
		pthread_t thread;
	public:
		Thread() : thread(0) {}
		virtual ~Thread() { stop(); }
		void start();
		void stop();
		void sleep( int millis );
		const bool running() { return (thread != 0); }
		virtual void run() = 0;
	};
	
	inline void *serve_thread( void *prm ) {
		Thread *t = (Thread*)prm;
		t->run();
		return 0;
	}
	
	inline void Thread::start() {
		pthread_create( &thread, 0, serve_thread, this );
	}
	
	inline void Thread::stop() {
		if ( thread ) 
			pthread_exit( 0 );
		thread = 0;
	}
	
	inline void Thread::sleep( int millis ) {
		usleep( millis );
	}

#endif // ! _WIN32

};


#endif //__ENGINE_THREAD_H__

#ifndef __VIDEO_STREAM_H__
#define __VIDEO_STREAM_H__
#include <string>
#include <windows.h>//avoid include the "windows.h" before the "winsock2.h" 

namespace Memory
{
	class SharedMemory
	{

	public:

		SharedMemory();
		//SharedMemory& operate = (SharedMemory& rhs) = delete;

	
		static const char*  DEFAULT_STREAMNAME;
		HANDLE				fileMappingHandle;
		unsigned char*		fileMappingView;

		unsigned char*		data;

		int					width;
		int					height;
		int					stride;

		int					buffer_len;

		int					last_read;

		bool				is_opened;

		bool				isOpened();
		
		bool open(std::string streamName = DEFAULT_STREAMNAME);

		int read(unsigned char* buffer,  int size);

		//int write(byte[] bytData, int lngAddr, int lngSize);

		int getTailPos();
		
		int getHeadPos();
		
		void setHeadPos(int pos);

		void close();

#if _FILE_SOURCE
		FILE*			m_fp;
#endif

	};
};

#endif
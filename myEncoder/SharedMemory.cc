#include "SharedMemory.h"
#include "WinBase.h"
#include <iostream>

namespace Memory
{
	const char* SharedMemory::DEFAULT_STREAMNAME = "Eagle_VideoStream";

	SharedMemory::SharedMemory():is_opened(false){}

	bool SharedMemory::open(std::string file_name) {
#if _FILE_SOURCE
		m_fp = fopen(file_name.c_str(), "rb");
		if (m_fp) {
			width = MAX_IAMGE_WIDTH;
			height = MAX_IMAGE_HEIGHT;
			return is_opened = true;;
		}
#endif

		is_opened = false;
		fileMappingHandle = OpenFileMapping(PAGE_READONLY | PAGE_READWRITE, FALSE, file_name.c_str());
		if (NULL == fileMappingHandle) {
			DWORD err = GetLastError();
			printf("OpenFileMapping err:%d\n", err);
			return is_opened;
		}

		fileMappingView =(unsigned char*) MapViewOfFile(fileMappingHandle, PAGE_READONLY, 0, 0, 0);

		if (NULL == fileMappingView) {
			goto end;
		}

		char header[4 * 12] = {0};
		memcpy(header, fileMappingView, sizeof(header));

		memcpy(&width, &header[4], 4);
		memcpy(&height, &header[8], 4);
		memcpy(&stride, &header[16], 4);
		memcpy(&buffer_len, &header[20], 4);

		//PixelFormat pf = PixelFormats.Bgr24;

		std::cout << __FUNCTION__ <<  " width(" << width << "),height(" << height << "),stride(" << stride << "), buffer_len(" << buffer_len << ")"<< std::endl;
		if (buffer_len <= 0 || width <= 0 || height <= 0 || stride <= 0)
			goto end;

		return is_opened=true;
	end:
		close();
		return is_opened;
	}

	int SharedMemory::read(unsigned char* buffer, int size) {
#if _FILE_SOURCE
		int nByteRead = 0;
		if (0 >= (nByteRead=fread(buffer, 1, size, m_fp))) {
			fseek(m_fp, 0, SEEK_SET);
			return fread(buffer, 1, size, m_fp);
		}
		return nByteRead;
#endif


		int head = getHeadPos();
		int tail = getTailPos();

		int want = tail - 1;
		want = want < 0 ? buffer_len - 1 :want;

		if (last_read == want ) {
			//cout << "last_read:" << last_read << ",want:" << want << endl;
			return 0;
		}

		//cout << "get frame number:" << want << endl;
		int pos = 4096 + want * stride * height;
		//int pos = 48;
		if (fileMappingView && buffer) {
			memcpy(buffer, fileMappingView + pos, size);
#if 0
			setHeadPos((want_head +1) % buffer_len);

#endif
		}
		last_read = want;
		return size;
	}

	int SharedMemory::getTailPos() {
		int res = 0;
		if (fileMappingView) {
			memcpy(&res, fileMappingView+28, sizeof(int));
		}
		return res;
	}

	int SharedMemory::getHeadPos() {
		int res = 0;
		if (fileMappingView) {
			memcpy(&res, fileMappingView + 24, sizeof(int));
		}
		return res;
	}

	void SharedMemory::setHeadPos(int pos) {
		if (fileMappingView) {
			memcpy(fileMappingView + 24, &pos, sizeof(int));
		}
	}

	bool SharedMemory::isOpened() {
		return is_opened;
	}

	void SharedMemory::close() {
#if _FILE_SOURCE
		fclose(m_fp);
		return;
#endif
		if (fileMappingView)
			UnmapViewOfFile(fileMappingView);
		if (fileMappingHandle)
			CloseHandle(fileMappingHandle);
	}
}

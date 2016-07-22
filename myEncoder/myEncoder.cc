#include "myEncoder.h"
#include "msdk_codec.h"
#include "FFmpegWriter.h"
#include "SharedMemory.h"
#include "FFmpegStreamer.h"

#include "opencv2\opencv.hpp"
#include <signal.h>


#pragma comment(lib, "msdk_codec.lib")

#pragma comment(lib, "ws2_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "ffmpeg_writerd.lib")
#pragma comment(lib, "opencv_highgui246d.lib")
#pragma comment(lib, "opencv_imgproc246d.lib")
#pragma comment(lib, "opencv_core246d.lib")
#pragma comment(lib, "ffmpeg_streamerD.lib")
#else
#pragma comment(lib, "ffmpeg_writer.lib")
#pragma comment(lib, "opencv_highgui246.lib")
#pragma comment(lib, "opencv_imgproc246.lib")
#pragma comment(lib, "opencv_core246.lib")
#pragma comment(lib, "ffmpeg_streamer.lib")
#endif


#define MAX_CACHE_SECONDS		60*2
#define MAX_FPS					25

typedef struct _CustomStruct {
	CFFmpegWriter			m_ff_writer;
	CEncodeThread			m_encoder;
	CFFmpegStreamer			m_ff_streamer;
	Memory::SharedMemory	m_sm;
	char*					m_rgb_buf;
	int						m_rgb_buf_len;

	char*					m_rgb_buf2;
	int						m_rgb_buf_len2;

	IplImage*				m_rgb_image;
	IplImage*				m_yuv_image;

	int						m_width;
	int						m_height;

	char					m_record_file[255];

	std::map<int64_t, std::vector<std::vector<char>>> m_frames;

	char					m_url[2048];

	_CustomStruct() {
		m_rgb_buf = NULL;
		m_rgb_buf_len = 0;

		m_rgb_buf2 = NULL;
		m_rgb_buf_len2 = 0;

		m_rgb_image = m_yuv_image = NULL;

		memset(m_record_file, 0x00, sizeof m_record_file);
		memset(m_url, 0x00, sizeof m_url);
	}

}CustomStruct;


CustomStruct g_cs;
std::string g_paramter;

//read a frame data from ven
//rgb 2 yuv
int ReadNextFrame(void* buffer, int32_t buffer_len, void* ctx) {

	CustomStruct *p = (CustomStruct*)ctx;
	if (!p->m_sm.isOpened()) {
#if _FILE_SOURCE
		if (!p->m_sm.open("e:\\640x480.rgb")) {
			fprintf(stdout, "SharedMemory open failed!\n");
			return -1;
		}
#else 
		if (!p->m_sm.open()) {
			fprintf(stdout, "SharedMemory open failed!\n");
			return -1;
		}
#endif

		p->m_width = p->m_sm.width;
		p->m_height = p->m_sm.height;

		if (p->m_rgb_buf == NULL) {
			p->m_rgb_buf_len = p->m_width*p->m_height * 3;
			p->m_rgb_buf = new char[p->m_rgb_buf_len];
		}

		if (p->m_rgb_buf2 == NULL) {
			p->m_rgb_buf_len2 = p->m_width*p->m_height * 3;
			p->m_rgb_buf2 = new char[p->m_rgb_buf_len2];
		}

		if (p->m_rgb_image == NULL) {
			p->m_rgb_image = cvCreateImageHeader(cvSize(p->m_width, p->m_height), 8, 3);
			cvSetData(p->m_rgb_image, p->m_rgb_buf2, p->m_width*3);
		}

		if (p->m_yuv_image == NULL) {
			p->m_yuv_image = cvCreateImage(cvSize(p->m_width, p->m_height * 3 / 2), 8, 1);
		}
	}

#if 1
	int64_t now1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	if (0 == p->m_sm.read((unsigned char*)p->m_rgb_buf, p->m_rgb_buf_len)) {
		return 0;
	}
	int64_t now2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	//上下颠倒一下rgb数据
	int stride = p->m_width * 3;
	for (int i = p->m_height - 1, j = 0; i > 0; --i, ++j) {
		memcpy(&p->m_rgb_buf2[stride * (i - 1)], &p->m_rgb_buf[stride * j], stride);
	}

	int64_t now3 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	//rgb 2 yuv
	cvCvtColor(p->m_rgb_image, p->m_yuv_image, CV_RGB2YUV_I420);
	int64_t now4 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	//cvShowImage("win", p->m_yuv_image);
	//cvWaitKey(1);

	if (buffer_len >= p->m_yuv_image->imageSize) {
		memcpy(buffer, p->m_yuv_image->imageData, p->m_yuv_image->imageSize);
		int64_t now5 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		//fprintf(stdout, "%s time read:%lld downup:%lld, cvt:%lld, cp2:%lld\n", __FUNCTION__, now2 - now1, now3-now2, now4-now3, now5-now4);

		std::this_thread::sleep_for(std::chrono::milliseconds(1000 / 25 - (now5 - now1)));
		return p->m_yuv_image->imageSize;
	}
#else 
	int64_t now6 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	if (buffer_len == p->m_sm.read( (unsigned char*)buffer, buffer_len)) {
		int64_t now7 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		fprintf(stdout, "%s time read:%lld \n", __FUNCTION__, now7 - now6);
		return buffer_len;
	}
#endif
	return 0;
}


int32_t WriteNextFrame(unsigned char* buffer, int32_t buffer_len, void*  ctx) {
	int nBytesWritten = 0;
#if 0
	static int i = 0;
	char name[100];
	sprintf(name, "e:\\%d.264", i++);
	FILE* fp = fopen(name, "wb+");
	if (fp) {
		fwrite(buffer, 1, buffer_len, fp);
		fclose(fp);
	}
#else

	CustomStruct *p = (CustomStruct*)ctx;
	CFFmpegWriter* p_writer = (CFFmpegWriter*)(&p->m_ff_writer);

	//cache every frame
	int64_t now = time(NULL);// std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	g_cs.m_frames[now].push_back(std::vector<char>(buffer, buffer + buffer_len));//FIXME: maybe race condition!!!

	if (now - g_cs.m_frames.begin()->first >= MAX_CACHE_SECONDS) {
		g_cs.m_frames.erase(g_cs.m_frames.begin());
		printf("%d data deleted!\n", g_cs.m_frames.begin()->first);
	}

	//check wether write videos
	if (strlen(g_cs.m_record_file) > 0) {
		int nLen = p_writer->write_video_frame(buffer, buffer_len, buffer[5] == 0x10);
		if (0 != nLen) {//success
			fprintf(stdout, "write_video_frame failed!\n");
		}
		nBytesWritten = buffer_len;
	}

	if (strlen(g_cs.m_url) > 0) {
		static bool flag = true;
		if (flag) {
			if (buffer[5] == 0x10) {
				flag = false;
				FILE* fp = fopen("tmp.264", "wb");
				if (fp) {
					fwrite(buffer, 1, buffer_len, fp);
					fclose(fp);
				}
				g_cs.m_ff_streamer.connect(g_cs.m_url);
			}
		}

		if (0 == g_cs.m_ff_streamer.send_video_frame(buffer, buffer_len, buffer[5] == 0x10)) {
			nBytesWritten = buffer_len;
		}
	}
#endif
	return nBytesWritten;
}

ENCODER_API bool encoder_rtmp_push(char* url) {
	strcpy(g_cs.m_url, url);
	return true;
}


ENCODER_API bool encoder_start(char* parameters, char* record_file_name) {
	//std::string paramter("-g 1920x1080 -b 3000 -f 25/1 -gop 25");
	//std::string paramter("-g 1920x1080 -b 3000 -f 30/1 -gop 45");
	g_paramter.assign(parameters);

	if (record_file_name != NULL) {
		strcpy(g_cs.m_record_file, record_file_name);
		g_cs.m_ff_writer.initialize(g_paramter);
		g_cs.m_ff_writer.create_video_file(record_file_name);
	}

	g_cs.m_encoder.init(g_paramter.data());
	g_cs.m_encoder.start(std::bind(ReadNextFrame, std::placeholders::_1, std::placeholders::_2, &g_cs), std::bind(WriteNextFrame, std::placeholders::_1, std::placeholders::_2, &g_cs));

	return true;
}

ENCODER_API bool encoder_stop() {
	g_cs.m_encoder.stop();

	g_cs.m_ff_writer.close_video_file();
	g_cs.m_ff_writer.uninitialize();
	g_cs.m_ff_streamer.close();
	
	cvReleaseImageHeader(&g_cs.m_rgb_image);
	cvReleaseImageHeader(&g_cs.m_yuv_image);

	delete[] g_cs.m_rgb_buf;
	delete[] g_cs.m_rgb_buf2;

	return true;
}

void myrun(char* file_name, int time_span, void* ctx) {
	CustomStruct* p = (CustomStruct*)ctx;
	int64_t now = time(NULL);// std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	int64_t start = now - time_span-2;//FIXME:
	if (p->m_frames.find(start) == p->m_frames.end()) {
		return;
	}

	
	CFFmpegWriter ffwriter;
	ffwriter.initialize(g_paramter);
	ffwriter.create_video_file(file_name);
	bool wait_key = true;

	while (start < now) {
		for (int i = 0; i < g_cs.m_frames[start].size(); ++i) {
			if (wait_key ) {
				if (g_cs.m_frames[start][i].data()[5] == 0x10) {
					wait_key = false;
				}
				else {
					continue;
				}
			}
			ffwriter.write_video_frame((unsigned char*)g_cs.m_frames[start][i].data(), g_cs.m_frames[start][i].size(), g_cs.m_frames[start][i].data()[5] == 0x10);
		}

		++start;
	}
	ffwriter.close_video_file();
	ffwriter.uninitialize();

	fprintf(stdout, "%s exited!\n", __FUNCTION__);
}

ENCODER_API bool encoder_output(char* file_name, int time_span) {
	if (file_name != NULL && strlen(file_name) > 0) {
		std::thread mythread(myrun, file_name, time_span, &g_cs);
		mythread.detach();
		return true;
	}

	return false;
}

bool g_running_flag = true;
void sig_cb(int sig)
{
	if (sig == SIGINT) {
		fprintf(stdout, "%s\n", __FUNCTION__);
		g_running_flag = false;
	}
}


int main(int argc, char** argv) {
	signal(SIGINT, sig_cb);  /*注册ctrl+c信号捕获函数*/

	//rtmp://localhost/publishlive/livestream;
	//char* rtmp_push_url = "rtmp://2453.livepush.myqcloud.com/live/2453_993e47c3f64311e5b91fa4dcbef5e35a?bizid=2453";
	//if (false == encoder_rtmp_push(rtmp_push_url)) {
	//	fprintf(stderr, "encoder_rtmp_push failed!\n");
	//}

	encoder_start("-g 640x480 -b 3000 -f 25/1 -gop 25","E:\\1.MP4");
	int64_t now1 = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	while (g_running_flag) {
		Sleep(100);
		int64_t now2 = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		if (now2 - now1 > 30) {
			now1 = now2;
			char name[256];
			sprintf(name, "e:\\svn20160118\\BilliardTrain\\bin\\Debug\\0001-1-1 0#00#00_0_0_0_0_0_0_0_0_00000000-0000-0000-0000-000000000000_{%d}.mp4", now2);
			encoder_output(name, 20);
		}
	}

	encoder_stop();
	return 0;
}

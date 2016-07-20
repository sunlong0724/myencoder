#include "Rtmp264.h"
#include "myEncoder.h"
#include "msdk_codec.h"
#include "FFmpegWriter.h"
#include "SharedMemory.h"

#include "opencv2\opencv.hpp"
#include <signal.h>


#pragma comment(lib, "msdk_codec.lib")
#pragma comment(lib, "librtmp.lib")
#pragma comment(lib, "ws2_32.lib")
#ifdef _DEBUG
#pragma comment(lib, "ffmpeg_writerd.lib")
#pragma comment(lib, "opencv_highgui246d.lib")
#pragma comment(lib, "opencv_imgproc246d.lib")
#pragma comment(lib, "opencv_core246d.lib")
#else
#pragma comment(lib, "ffmpeg_writer.lib")
#pragma comment(lib, "opencv_highgui246.lib")
#pragma comment(lib, "opencv_imgproc246.lib")
#pragma comment(lib, "opencv_core246.lib")
#endif


#define MAX_CACHE_SECONDS		60*5
typedef struct _CustomStruct {
	CFFmpegWriter			m_ff_writer;
	CEncodeThread			m_encoder;
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

	char					m_url[256];
	CRtmp264				m_rtmp264;
	int						m_rtmp_connect_status;

	_CustomStruct() {
		m_rgb_buf = NULL;
		m_rgb_buf_len = 0;

		m_rgb_buf2 = NULL;
		m_rgb_buf_len2 = 0;

		m_rgb_image = m_yuv_image = NULL;

		memset(m_record_file, 0x00, sizeof m_record_file);
		memset(m_url, 0x00, sizeof m_url);
		m_rtmp_connect_status = FALSE;
	}

}CustomStruct;

CustomStruct g_cs;
std::string paramter("-g 1920x1080 -b 3000 -f 25/1 -gop 25");

//read a frame data from ven
//rgb 2 yuv
int ReadNextFrame(void* buffer, int32_t buffer_len, void* ctx) {

	CustomStruct *p = (CustomStruct*)ctx;
	if (!p->m_sm.isOpened()) {
#if _FILE_SOURCE
		if (!p->m_sm.open("e:\\1.yuv")) {
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

#if 0
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
		fprintf(stdout, "%s time read:%lld downup:%lld, cvt:%lld, cp2:%lld\n", __FUNCTION__, now2 - now1, now3-now2, now4-now3, now5-now4);
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
	int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
	g_cs.m_frames[now].push_back(std::vector<char>(buffer, buffer + buffer_len));//FIXME: maybe race condition!!!


	//check wether write videos
	if (strlen(g_cs.m_record_file) > 0) {
		int nLen = p_writer->write_video_frame(buffer, buffer_len, buffer[5] == 0x10);
		if (0 != nLen) {//success
			fprintf(stdout, "write_video_frame failed!\n");
		}
	}

	//check whether send
	//first come must be I
	if (p->m_rtmp_connect_status) {
		int64_t now1 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
		p->m_rtmp264.RTMP264_Send(buffer, buffer_len);
		int64_t now2 = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

		//fprintf(stdout, "%s RTMP264_Send(%d),time(%lld)\n", __FUNCTION__, i,now2 - now1);
	}
	else {
		if (strlen(p->m_url) > 0) {
			int ret = p->m_rtmp264.RTMP264_Connect(p->m_url);
			fprintf(stdout, "%s RTMP264_Connect %d\n", __FUNCTION__, ret);
		}
	}
#endif
	return buffer_len;
}

ENCODER_API bool encoder_rtmp_push(char* url) {
	return g_cs.m_rtmp_connect_status = g_cs.m_rtmp264.RTMP264_Connect(url);
}


ENCODER_API bool encoder_start(char* record_file_name) {
	//std::string paramter("-g 1920x1080 -b 3000 -f 25/1 -gop 25");
	//std::string paramter("-g 1920x1080 -b 3000 -f 30/1 -gop 45");
	if (record_file_name != NULL) {
		strcpy(g_cs.m_record_file, record_file_name);
		g_cs.m_ff_writer.initialize(paramter);
		g_cs.m_ff_writer.create_video_file(record_file_name);
	}

	g_cs.m_encoder.init(paramter.data());
	g_cs.m_encoder.start(std::bind(ReadNextFrame, std::placeholders::_1, std::placeholders::_2, &g_cs), std::bind(WriteNextFrame, std::placeholders::_1, std::placeholders::_2, &g_cs));

	return true;
}

ENCODER_API bool encoder_stop() {
	g_cs.m_encoder.stop();
	g_cs.m_rtmp264.RTMP264_Close();
	g_cs.m_ff_writer.close_video_file();
	g_cs.m_ff_writer.uninitialize();
	
	cvReleaseImageHeader(&g_cs.m_rgb_image);
	cvReleaseImageHeader(&g_cs.m_yuv_image);

	delete[] g_cs.m_rgb_buf;
	delete[] g_cs.m_rgb_buf2;

	return true;
}

void myrun(char* file_name, int time_span, void* ctx) {
	CustomStruct* p = (CustomStruct*)ctx;
	int64_t now = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

	int64_t start = now - time_span;
	if (p->m_frames.find(now) == p->m_frames.end()) {
		return;
	}

	
	CFFmpegWriter ffwriter;
	ffwriter.initialize(paramter);
	ffwriter.create_video_file(file_name);

	while (start < now) {
		for (int i = 0; i < g_cs.m_frames[start].size(); ++i) {
			ffwriter.write_video_frame((unsigned char*)g_cs.m_frames[start][i].data(), g_cs.m_frames[start][i].size(), g_cs.m_frames[start][i].data()[5] == 0x10);
		}
		if (now - g_cs.m_frames.begin()->first >= MAX_CACHE_SECONDS) {
			g_cs.m_frames.erase(start);
		}
		++start;
	}
	ffwriter.close_video_file();
	ffwriter.uninitialize();

	fprintf(stdout, "%s exited!\n", __FUNCTION__);
}

ENCODER_API bool encoder_output(char* file_name, int time_span) {
	std::thread mythread(myrun,file_name, time_span, &g_cs);
	mythread.detach();
	return true;
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
	char* rtmp_push_url = "rtmp://2453.livepush.myqcloud.com/live/2453_993e47c3f64311e5b91fa4dcbef5e35a?bizid=2453";
	if (false == encoder_rtmp_push(rtmp_push_url)) {
		fprintf(stderr, "encoder_rtmp_push failed!\n");
	}

	encoder_start("E:\\1.MP4");
	//encoder_start("");
	while (g_running_flag) {
		Sleep(500);
	}

	encoder_stop();
	return 0;
}

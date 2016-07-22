#pragma once
#define DLL_API __declspec(dllexport)  

#include <string>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/avutil.h"
#include "libavutil/imgutils.h"
#include "libswscale/swscale.h"
#include "libavutil/time.h"
#include "libavutil/opt.h"
}

class DLL_API CFFmpegStreamer {
public:
	int32_t connect(const std::string& url);
	int32_t close();
	int32_t send_video_frame(unsigned char* buf, int len, bool is_key_frame);

private:
	AVFormatContext*	m_input_fmt_ctx;
	AVFormatContext*	m_output_fmt_ctx;
	AVStream*			m_input_stream;
	AVStream*			m_output_stream;

	int32_t				m_frame_count;
	int64_t				m_start_time;
	static  bool		m_register_ffmpeg;

	int					m_video_index;
};



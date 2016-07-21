#ifndef __MY_ENCODER_H__
#define __MY_ENCODER_H__

#ifdef EXPORT_API_DLL
#define ENCODER_API   __declspec(dllexport)
#else
#define ENCODER_API __declspec(dllimport)
#endif

extern "C" {

	ENCODER_API bool encoder_rtmp_push(char* addr);

	ENCODER_API bool encoder_start(char* parameters,char* record_file_name);

	ENCODER_API bool encoder_stop();

	ENCODER_API bool encoder_output(char* file_name, int time_span);



}

#endif
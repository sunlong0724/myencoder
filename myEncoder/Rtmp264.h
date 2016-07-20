#ifndef __RTMP_264_H__
#define __RTMP_264_H__
#include "librtmp\rtmp.h"   
#include "librtmp\rtmp_sys.h"   
#include "librtmp\amf.h"  


#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)


/**
* _RTMPMetadata
* 内部结构体。该结构体主要用于存储和传递元数据信息
*/
typedef struct _RTMPMetadata
{
	// video, must be h264 type   
	unsigned int    nWidth;
	unsigned int    nHeight;
	unsigned int    nFrameRate;
	unsigned int    nSpsLen;
	unsigned char   *Sps;
	unsigned int    nPpsLen;
	unsigned char   *Pps;
	unsigned char	*sei;
	unsigned int	nSeiLen;
	int				level_idc;
} RTMPMetadata, *LPRTMPMetadata;

/**
* _NaluUnit
* 内部结构体。该结构体主要用于存储和传递Nal单元的类型、大小和数据
*/
typedef struct _NaluUnit
{
	int type;
	int size;
	unsigned char *data;
}NaluUnit;

class CRtmp264 {

private:
	/**
	* 初始化winsock
	*
	* @成功则返回1 , 失败则返回相应错误代码
	*/
	static int InitSockets();

	/**
	* 释放winsock
	*
	* @成功则返回0 , 失败则返回相应错误代码
	*/
	static  void CleanupSockets();


	/**
	* 解码SPS,获取视频图像宽、高信息
	*
	* @param buf SPS数据内容
	* @param nLen SPS数据的长度
	* @param width 图像宽度
	* @param height 图像高度

	* @成功则返回1 , 失败则返回0
	*/
	int h264_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height, int &fps, int& level_idc);
	UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit);
	int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit);
	DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit);
	void peekSpsAndPps(unsigned char* buf, int buf_size);

	/**
	* 发送RTMP数据包
	*
	* @param nPacketType 数据类型
	* @param data 存储数据内容
	* @param size 数据大小
	* @param nTimestamp 当前包的时间戳
	*
	* @成功则返回 1 , 失败则返回一个小于0的数
	*/
	int SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp);

	/**
	* 发送视频的sps和pps信息
	*
	* @param pps 存储视频的pps信息
	* @param pps_len 视频的pps信息长度
	* @param sps 存储视频的pps信息
	* @param sps_len 视频的sps信息长度
	*
	* @成功则返回 1 , 失败则返回0
	*/
	int SendVideoSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len);

	/**
	* 发送H264数据帧
	*
	* @param data 存储数据帧内容
	* @param size 数据帧的大小
	* @param bIsKeyFrame 记录该帧是否为关键帧
	* @param nTimeStamp 当前帧的时间戳
	*
	* @成功则返回 1 , 失败则返回0
	*/
	int SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp);

	/**
	* H264的NAL起始码防竞争机制
	*
	* @param buf SPS数据内容
	*
	* @无返回值
	*/
	void de_emulation_prevention(BYTE* buf, unsigned int* buf_size);

public:
	/**
	* 初始化并连接到服务器
	*
	* @param url 服务器上对应webapp的地址
	*
	* @成功则返回1 , 失败则返回0
	*/
	int RTMP264_Connect(const char* url);

	/**
	* 断开连接，释放相关的资源。
	*
	*/
	void RTMP264_Close();

	/**
	* 将内存中的一段H.264编码的视频数据利用RTMP协议发送到服务器
	*
	* @param read_buffer 回调函数，当数据不足的时候，系统会自动调用该函数获取输入数据。
	*					2个参数功能：
	*					uint8_t *buf：外部数据送至该地址
	*					int buf_size：外部数据大小
	*					返回值：成功读取的内存大小
	* @成功则返回1 , 失败则返回0
	*/
	int RTMP264_Send(unsigned char *buf, int buf_size);
	

private:
	RTMP*			m_pRtmp;
	RTMPMetadata	m_metaData;
	int				m_tick;
	int				m_tick_gap;
};
#endif // !__RTMP_264_H__



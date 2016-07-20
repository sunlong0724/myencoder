#ifndef __RTMP_264_H__
#define __RTMP_264_H__
#include "librtmp\rtmp.h"   
#include "librtmp\rtmp_sys.h"   
#include "librtmp\amf.h"  


#define RTMP_HEAD_SIZE   (sizeof(RTMPPacket)+RTMP_MAX_HEADER_SIZE)


/**
* _RTMPMetadata
* �ڲ��ṹ�塣�ýṹ����Ҫ���ڴ洢�ʹ���Ԫ������Ϣ
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
* �ڲ��ṹ�塣�ýṹ����Ҫ���ڴ洢�ʹ���Nal��Ԫ�����͡���С������
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
	* ��ʼ��winsock
	*
	* @�ɹ��򷵻�1 , ʧ���򷵻���Ӧ�������
	*/
	static int InitSockets();

	/**
	* �ͷ�winsock
	*
	* @�ɹ��򷵻�0 , ʧ���򷵻���Ӧ�������
	*/
	static  void CleanupSockets();


	/**
	* ����SPS,��ȡ��Ƶͼ�������Ϣ
	*
	* @param buf SPS��������
	* @param nLen SPS���ݵĳ���
	* @param width ͼ����
	* @param height ͼ��߶�

	* @�ɹ��򷵻�1 , ʧ���򷵻�0
	*/
	int h264_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height, int &fps, int& level_idc);
	UINT Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit);
	int Se(BYTE *pBuff, UINT nLen, UINT &nStartBit);
	DWORD u(UINT BitCount, BYTE * buf, UINT &nStartBit);
	void peekSpsAndPps(unsigned char* buf, int buf_size);

	/**
	* ����RTMP���ݰ�
	*
	* @param nPacketType ��������
	* @param data �洢��������
	* @param size ���ݴ�С
	* @param nTimestamp ��ǰ����ʱ���
	*
	* @�ɹ��򷵻� 1 , ʧ���򷵻�һ��С��0����
	*/
	int SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp);

	/**
	* ������Ƶ��sps��pps��Ϣ
	*
	* @param pps �洢��Ƶ��pps��Ϣ
	* @param pps_len ��Ƶ��pps��Ϣ����
	* @param sps �洢��Ƶ��pps��Ϣ
	* @param sps_len ��Ƶ��sps��Ϣ����
	*
	* @�ɹ��򷵻� 1 , ʧ���򷵻�0
	*/
	int SendVideoSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len);

	/**
	* ����H264����֡
	*
	* @param data �洢����֡����
	* @param size ����֡�Ĵ�С
	* @param bIsKeyFrame ��¼��֡�Ƿ�Ϊ�ؼ�֡
	* @param nTimeStamp ��ǰ֡��ʱ���
	*
	* @�ɹ��򷵻� 1 , ʧ���򷵻�0
	*/
	int SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp);

	/**
	* H264��NAL��ʼ�����������
	*
	* @param buf SPS��������
	*
	* @�޷���ֵ
	*/
	void de_emulation_prevention(BYTE* buf, unsigned int* buf_size);

public:
	/**
	* ��ʼ�������ӵ�������
	*
	* @param url �������϶�Ӧwebapp�ĵ�ַ
	*
	* @�ɹ��򷵻�1 , ʧ���򷵻�0
	*/
	int RTMP264_Connect(const char* url);

	/**
	* �Ͽ����ӣ��ͷ���ص���Դ��
	*
	*/
	void RTMP264_Close();

	/**
	* ���ڴ��е�һ��H.264�������Ƶ��������RTMPЭ�鷢�͵�������
	*
	* @param read_buffer �ص������������ݲ����ʱ��ϵͳ���Զ����øú�����ȡ�������ݡ�
	*					2���������ܣ�
	*					uint8_t *buf���ⲿ���������õ�ַ
	*					int buf_size���ⲿ���ݴ�С
	*					����ֵ���ɹ���ȡ���ڴ��С
	* @�ɹ��򷵻�1 , ʧ���򷵻�0
	*/
	int RTMP264_Send(unsigned char *buf, int buf_size);
	

private:
	RTMP*			m_pRtmp;
	RTMPMetadata	m_metaData;
	int				m_tick;
	int				m_tick_gap;
};
#endif // !__RTMP_264_H__



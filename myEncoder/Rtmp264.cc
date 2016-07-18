
#include "Rtmp264.h"
#include <math.h>

int CRtmp264::InitSockets(){
#ifdef WIN32     
	WORD version;
	WSADATA wsaData;
	version = MAKEWORD(1, 1);
	return (WSAStartup(version, &wsaData) == 0);
#else     
	return TRUE;
#endif     
}

void CRtmp264::CleanupSockets()
{
#ifdef WIN32     
	WSACleanup();
#endif     
}

int CRtmp264::RTMP264_Connect(const char* url)	{
	InitSockets();
	m_get_sps_pps = false;
	m_pRtmp = NULL;
	m_tick = 0;
	m_tick_gap = 0;

	m_pRtmp = RTMP_Alloc();
	RTMP_Init(m_pRtmp);
	/*设置URL*/
	if (RTMP_SetupURL(m_pRtmp, (char*)url) == FALSE)
	{
		RTMP_Free(m_pRtmp);
		return false;
	}
	/*设置可写,即发布流,这个函数必须在连接前使用,否则无效*/
	RTMP_EnableWrite(m_pRtmp);
	/*连接服务器*/
	if (RTMP_Connect(m_pRtmp, NULL) == FALSE)
	{
		RTMP_Free(m_pRtmp);
		return false;
	}

	/*连接流*/
	if (RTMP_ConnectStream(m_pRtmp, 0) == FALSE)
	{
		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		return false;
	}
	return true;
}

void CRtmp264::RTMP264_Close()
{
	if (m_pRtmp)
	{
		if (m_metaData.Pps) {
			delete[] m_metaData.Pps;
		}
		if (m_metaData.Sps) {
			delete[] m_metaData.Sps;
		}

		RTMP_Close(m_pRtmp);
		RTMP_Free(m_pRtmp);
		m_pRtmp = NULL;
	}
	CleanupSockets();
}

void CRtmp264::peekSpsAndPps(unsigned char* buf, int buf_size) {
	int i = 0;
	int sps_start = 0;
	int sps_end = 0;
	int pps_start = 0;
	int pps_end = 0;
	for (int i = 0; i < buf_size; ++i) {
		if (buf[i] == 0x00 && buf[i + 1] == 0x00 && buf[i + 2] == 0x00 && buf[i + 3] == 0x01) {
			if (buf[i + 4] == 0x27) {//sps
				sps_start = i;
				continue;
			}
			else if (buf[i + 4] == 0x28) {//pps
				pps_start = sps_end = i;
				continue;
			}
			else if (buf[i + 4] == 0x06) {//sei and slice
				pps_end = i;
				break;
			}
		}
	}
	m_metaData.nSpsLen = sps_end - sps_start;
	m_metaData.Sps = NULL;
	m_metaData.Sps = (unsigned char*)malloc(m_metaData.nSpsLen);
	memcpy(m_metaData.Sps, &buf[sps_start], m_metaData.nSpsLen);


	m_metaData.nPpsLen = pps_end - pps_start;
	m_metaData.Pps = NULL;
	m_metaData.Pps = (unsigned char*)malloc(m_metaData.nPpsLen);
	memcpy(m_metaData.Pps, &buf[pps_start], m_metaData.nPpsLen);
}

int CRtmp264:: RTMP264_Send(unsigned char *buf, int buf_size){
	int bKeyframe = buf[5] == 0x10 ? TRUE : FALSE;
	if (m_get_sps_pps) {
		memset(&m_metaData, 0, sizeof(RTMPMetadata));
		peekSpsAndPps(buf, buf_size);

		// 解码SPS,获取视频图像宽、高信息   
		int width = 0, height = 0, fps = 0;
		h264_decode_sps(m_metaData.Sps, m_metaData.nSpsLen, width, height, fps);
		//m_metaData.nWidth = width;  
		//m_metaData.nHeight = height;  
		if (fps)
			m_metaData.nFrameRate = fps;
		else
			m_metaData.nFrameRate = 25;
			
		//except aud
		return SendH264Packet(buf+6, buf_size-6, bKeyframe, m_tick);
	}

	m_tick += m_tick_gap;
	return SendH264Packet(buf+6, buf_size-6, bKeyframe, m_tick);
}


int CRtmp264::SendPacket(unsigned int nPacketType, unsigned char *data, unsigned int size, unsigned int nTimestamp){
	RTMPPacket* packet;
	/*分配包内存和初始化,len为包体长度*/
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + size);
	memset(packet, 0, RTMP_HEAD_SIZE);
	/*包体内存*/
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	packet->m_nBodySize = size;
	memcpy(packet->m_body, data, size);
	packet->m_hasAbsTimestamp = 0;
	packet->m_packetType = nPacketType; /*此处为类型有两种一种是音频,一种是视频*/
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;
	packet->m_nChannel = 0x04;

	packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
	if (RTMP_PACKET_TYPE_AUDIO == nPacketType && size != 4)
	{
		packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	}
	packet->m_nTimeStamp = nTimestamp;
	/*发送*/
	int nRet = 0;
	if (RTMP_IsConnected(m_pRtmp))
	{
		nRet = RTMP_SendPacket(m_pRtmp, packet, TRUE); /*TRUE为放进发送队列,FALSE是不放进发送队列,直接发送*/
	}
	/*释放内存*/
	free(packet);
	return nRet;
}

int CRtmp264::SendVideoSpsPps(unsigned char *pps, int pps_len, unsigned char * sps, int sps_len){
	RTMPPacket * packet = NULL;//rtmp包结构
	unsigned char * body = NULL;
	int i;
	packet = (RTMPPacket *)malloc(RTMP_HEAD_SIZE + 1024);
	//RTMPPacket_Reset(packet);//重置packet状态
	memset(packet, 0, RTMP_HEAD_SIZE + 1024);
	packet->m_body = (char *)packet + RTMP_HEAD_SIZE;
	body = (unsigned char *)packet->m_body;
	i = 0;
	body[i++] = 0x17;
	body[i++] = 0x00;

	body[i++] = 0x00;
	body[i++] = 0x00;
	body[i++] = 0x00;

	/*AVCDecoderConfigurationRecord*/
	body[i++] = 0x01;
	body[i++] = sps[1];
	body[i++] = sps[2];
	body[i++] = sps[3];
	body[i++] = 0xff;

	/*sps*/
	body[i++] = 0xe1;
	body[i++] = (sps_len >> 8) & 0xff;
	body[i++] = sps_len & 0xff;
	memcpy(&body[i], sps, sps_len);
	i += sps_len;

	/*pps*/
	body[i++] = 0x01;
	body[i++] = (pps_len >> 8) & 0xff;
	body[i++] = (pps_len)& 0xff;
	memcpy(&body[i], pps, pps_len);
	i += pps_len;

	packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
	packet->m_nBodySize = i;
	packet->m_nChannel = 0x04;
	packet->m_nTimeStamp = 0;
	packet->m_hasAbsTimestamp = 0;
	packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
	packet->m_nInfoField2 = m_pRtmp->m_stream_id;

	/*调用发送接口*/
	int nRet = RTMP_SendPacket(m_pRtmp, packet, TRUE);
	free(packet);    //释放内存
	return nRet;
}

int CRtmp264::SendH264Packet(unsigned char *data, unsigned int size, int bIsKeyFrame, unsigned int nTimeStamp)
{
	if (data == NULL && size<11) {
		return false;
	}

	unsigned char *body = (unsigned char*)malloc(size + 9);
	memset(body, 0, size + 9);

	int i = 0;
	if (bIsKeyFrame) {
		body[i++] = 0x17;// 1:Iframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;


		// NALU size   
		body[i++] = size >> 24 & 0xff;
		body[i++] = size >> 16 & 0xff;
		body[i++] = size >> 8 & 0xff;
		body[i++] = size & 0xff;
		// NALU data   
		memcpy(&body[i], data, size);
		SendVideoSpsPps(m_metaData.Pps, m_metaData.nPpsLen, m_metaData.Sps, m_metaData.nSpsLen);
	}
	else {
		body[i++] = 0x27;// 2:Pframe  7:AVC   
		body[i++] = 0x01;// AVC NALU   
		body[i++] = 0x00;
		body[i++] = 0x00;
		body[i++] = 0x00;


		// NALU size   
		body[i++] = size >> 24 & 0xff;
		body[i++] = size >> 16 & 0xff;
		body[i++] = size >> 8 & 0xff;
		body[i++] = size & 0xff;
		// NALU data   
		memcpy(&body[i], data, size);
	}


	int bRet = SendPacket(RTMP_PACKET_TYPE_VIDEO, body, i + size, nTimeStamp);

	free(body);

	return bRet;
}



void CRtmp264::de_emulation_prevention(BYTE* buf, unsigned int* buf_size)
{
	int i = 0, j = 0;
	BYTE* tmp_ptr = NULL;
	unsigned int tmp_buf_size = 0;
	int val = 0;

	tmp_ptr = buf;
	tmp_buf_size = *buf_size;
	for (i = 0; i<(tmp_buf_size - 2); i++)
	{
		//check for 0x000003
		val = (tmp_ptr[i] ^ 0x00) + (tmp_ptr[i + 1] ^ 0x00) + (tmp_ptr[i + 2] ^ 0x03);
		if (val == 0)
		{
			//kick out 0x03
			for (j = i + 2; j<tmp_buf_size - 1; j++)
				tmp_ptr[j] = tmp_ptr[j + 1];

			//and so we should devrease bufsize
			(*buf_size)--;
		}
	}

	return;
}

int CRtmp264::h264_decode_sps(BYTE * buf, unsigned int nLen, int &width, int &height, int &fps)
{
	UINT StartBit = 0;
	fps = 0;
	de_emulation_prevention(buf, &nLen);

	int forbidden_zero_bit = u(1, buf, StartBit);
	int nal_ref_idc = u(2, buf, StartBit);
	int nal_unit_type = u(5, buf, StartBit);
	if (nal_unit_type == 7)
	{
		int profile_idc = u(8, buf, StartBit);
		int constraint_set0_flag = u(1, buf, StartBit);//(buf[1] & 0x80)>>7;
		int constraint_set1_flag = u(1, buf, StartBit);//(buf[1] & 0x40)>>6;
		int constraint_set2_flag = u(1, buf, StartBit);//(buf[1] & 0x20)>>5;
		int constraint_set3_flag = u(1, buf, StartBit);//(buf[1] & 0x10)>>4;
		int reserved_zero_4bits = u(4, buf, StartBit);
		int level_idc = u(8, buf, StartBit);

		int seq_parameter_set_id = Ue(buf, nLen, StartBit);

		if (profile_idc == 100 || profile_idc == 110 ||
			profile_idc == 122 || profile_idc == 144)
		{
			int chroma_format_idc = Ue(buf, nLen, StartBit);
			if (chroma_format_idc == 3)
				int residual_colour_transform_flag = u(1, buf, StartBit);
			int bit_depth_luma_minus8 = Ue(buf, nLen, StartBit);
			int bit_depth_chroma_minus8 = Ue(buf, nLen, StartBit);
			int qpprime_y_zero_transform_bypass_flag = u(1, buf, StartBit);
			int seq_scaling_matrix_present_flag = u(1, buf, StartBit);

			int seq_scaling_list_present_flag[8];
			if (seq_scaling_matrix_present_flag)
			{
				for (int i = 0; i < 8; i++) {
					seq_scaling_list_present_flag[i] = u(1, buf, StartBit);
				}
			}
		}
		int log2_max_frame_num_minus4 = Ue(buf, nLen, StartBit);
		int pic_order_cnt_type = Ue(buf, nLen, StartBit);
		if (pic_order_cnt_type == 0)
			int log2_max_pic_order_cnt_lsb_minus4 = Ue(buf, nLen, StartBit);
		else if (pic_order_cnt_type == 1)
		{
			int delta_pic_order_always_zero_flag = u(1, buf, StartBit);
			int offset_for_non_ref_pic = Se(buf, nLen, StartBit);
			int offset_for_top_to_bottom_field = Se(buf, nLen, StartBit);
			int num_ref_frames_in_pic_order_cnt_cycle = Ue(buf, nLen, StartBit);

			int *offset_for_ref_frame = new int[num_ref_frames_in_pic_order_cnt_cycle];
			for (int i = 0; i < num_ref_frames_in_pic_order_cnt_cycle; i++)
				offset_for_ref_frame[i] = Se(buf, nLen, StartBit);
			delete[] offset_for_ref_frame;
		}
		int num_ref_frames = Ue(buf, nLen, StartBit);
		int gaps_in_frame_num_value_allowed_flag = u(1, buf, StartBit);
		int pic_width_in_mbs_minus1 = Ue(buf, nLen, StartBit);
		int pic_height_in_map_units_minus1 = Ue(buf, nLen, StartBit);

		width = (pic_width_in_mbs_minus1 + 1) * 16;
		height = (pic_height_in_map_units_minus1 + 1) * 16;

		int frame_mbs_only_flag = u(1, buf, StartBit);
		if (!frame_mbs_only_flag)
			int mb_adaptive_frame_field_flag = u(1, buf, StartBit);

		int direct_8x8_inference_flag = u(1, buf, StartBit);
		int frame_cropping_flag = u(1, buf, StartBit);
		if (frame_cropping_flag)
		{
			int frame_crop_left_offset = Ue(buf, nLen, StartBit);
			int frame_crop_right_offset = Ue(buf, nLen, StartBit);
			int frame_crop_top_offset = Ue(buf, nLen, StartBit);
			int frame_crop_bottom_offset = Ue(buf, nLen, StartBit);
		}
		int vui_parameter_present_flag = u(1, buf, StartBit);
		if (vui_parameter_present_flag)
		{
			int aspect_ratio_info_present_flag = u(1, buf, StartBit);
			if (aspect_ratio_info_present_flag)
			{
				int aspect_ratio_idc = u(8, buf, StartBit);
				if (aspect_ratio_idc == 255)
				{
					int sar_width = u(16, buf, StartBit);
					int sar_height = u(16, buf, StartBit);
				}
			}
			int overscan_info_present_flag = u(1, buf, StartBit);
			if (overscan_info_present_flag)
				int overscan_appropriate_flagu = u(1, buf, StartBit);
			int video_signal_type_present_flag = u(1, buf, StartBit);
			if (video_signal_type_present_flag)
			{
				int video_format = u(3, buf, StartBit);
				int video_full_range_flag = u(1, buf, StartBit);
				int colour_description_present_flag = u(1, buf, StartBit);
				if (colour_description_present_flag)
				{
					int colour_primaries = u(8, buf, StartBit);
					int transfer_characteristics = u(8, buf, StartBit);
					int matrix_coefficients = u(8, buf, StartBit);
				}
			}
			int chroma_loc_info_present_flag = u(1, buf, StartBit);
			if (chroma_loc_info_present_flag)
			{
				int chroma_sample_loc_type_top_field = Ue(buf, nLen, StartBit);
				int chroma_sample_loc_type_bottom_field = Ue(buf, nLen, StartBit);
			}
			int timing_info_present_flag = u(1, buf, StartBit);
			if (timing_info_present_flag)
			{
				int num_units_in_tick = u(32, buf, StartBit);
				int time_scale = u(32, buf, StartBit);
				fps = time_scale / (2 * num_units_in_tick);
			}
		}
		return true;
	}
	else
		return false;
}

UINT CRtmp264::Ue(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	//计算0bit的个数
	UINT nZeroNum = 0;
	while (nStartBit < nLen * 8)
	{
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8))) //&:按位与，%取余
		{
			break;
		}
		nZeroNum++;
		nStartBit++;
	}
	nStartBit++;


	//计算结果
	DWORD dwRet = 0;
	for (UINT i = 0; i<nZeroNum; i++)
	{
		dwRet <<= 1;
		if (pBuff[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return (1 << nZeroNum) - 1 + dwRet;
}

int CRtmp264::Se(BYTE *pBuff, UINT nLen, UINT &nStartBit)
{
	int UeVal = Ue(pBuff, nLen, nStartBit);
	double k = UeVal;
	int nValue = ceil(k / 2);//ceil函数：ceil函数的作用是求不小于给定实数的最小整数。ceil(2)=ceil(1.2)=cei(1.5)=2.00
	if (UeVal % 2 == 0)
		nValue = -nValue;
	return nValue;
}


DWORD CRtmp264::u(UINT BitCount, BYTE * buf, UINT &nStartBit)
{
	DWORD dwRet = 0;
	for (UINT i = 0; i<BitCount; i++)
	{
		dwRet <<= 1;
		if (buf[nStartBit / 8] & (0x80 >> (nStartBit % 8)))
		{
			dwRet += 1;
		}
		nStartBit++;
	}
	return dwRet;
}

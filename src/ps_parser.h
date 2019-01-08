#ifndef __PS__PARSER__INC__HH__
#define __PS__PARSER__INC__HH__

#include "head.h"
#define MAX_PS_LENGTH  (0xFFFF)
#define MAX_PES_LENGTH (0xFFFF)
#define MAX_ES_LENGTH  (0x100000)
#ifndef AV_RB16
#define AV_RB16(x)                           \
    ((((const unsigned char*)(x))[0] << 8) |          \
    ((const unsigned char*)(x))[1])
#endif

#pragma pack (1)
typedef struct ps_header {
	unsigned char pack_start_code[4];  //'0x000001BA'

	unsigned char system_clock_reference_base21 : 2;
	unsigned char marker_bit : 1;
	unsigned char system_clock_reference_base1 : 3;
	unsigned char fix_bit : 2;    //'01'

	unsigned char system_clock_reference_base22;

	unsigned char system_clock_reference_base31 : 2;
	unsigned char marker_bit1 : 1;
	unsigned char system_clock_reference_base23 : 5;

	unsigned char system_clock_reference_base32;
	unsigned char system_clock_reference_extension1 : 2;
	unsigned char marker_bit2 : 1;
	unsigned char system_clock_reference_base33 : 5; //system_clock_reference_base 33bit

	unsigned char marker_bit3 : 1;
	unsigned char system_clock_reference_extension2 : 7; //system_clock_reference_extension 9bit

	unsigned char program_mux_rate1;

	unsigned char program_mux_rate2;
	unsigned char marker_bit5 : 1;
	unsigned char marker_bit4 : 1;
	unsigned char program_mux_rate3 : 6;

	unsigned char pack_stuffing_length : 3;
	unsigned char reserved : 5;
}ps_header_t;  //14

typedef struct sh_header
{
	unsigned char system_header_start_code[4]; //32

	unsigned char header_length[2];            //16 uimsbf

	uint32_t marker_bit1 : 1;   //1  bslbf
	uint32_t rate_bound : 22;   //22 uimsbf
	uint32_t marker_bit2 : 1;   //1 bslbf
	uint32_t audio_bound : 6;   //6 uimsbf
	uint32_t fixed_flag : 1;    //1 bslbf
	uint32_t CSPS_flag : 1;     //1 bslbf

	uint16_t system_audio_lock_flag : 1;  // bslbf
	uint16_t system_video_lock_flag : 1;  // bslbf
	uint16_t marker_bit3 : 1;             // bslbf
	uint16_t video_bound : 5;             // uimsbf
	uint16_t packet_rate_restriction_flag : 1; //bslbf
	uint16_t reserved_bits : 7;                //bslbf
	unsigned char reserved[6];
}sh_header_t; //18

typedef struct psm_header {
	unsigned char promgram_stream_map_start_code[4];

	unsigned char program_stream_map_length[2];

	unsigned char program_stream_map_version : 5;
	unsigned char reserved1 : 2;
	unsigned char current_next_indicator : 1;

	unsigned char marker_bit : 1;
	unsigned char reserved2 : 7;

	unsigned char program_stream_info_length[2];
	unsigned char elementary_stream_map_length[2];
	unsigned char stream_type;
	unsigned char elementary_stream_id;
	unsigned char elementary_stream_info_length[2];
	unsigned char CRC_32[4];
	unsigned char reserved[16];
}psm_header_t; //36

typedef struct pes_header
{
	unsigned char pes_start_code_prefix[3];
	unsigned char stream_id;
	unsigned short PES_packet_length;
}pes_header_t; //6

typedef struct optional_pes_header {
	unsigned char original_or_copy : 1;
	unsigned char copyright : 1;
	unsigned char data_alignment_indicator : 1;
	unsigned char PES_priority : 1;
	unsigned char PES_scrambling_control : 2;
	unsigned char fix_bit : 2;

	unsigned char PES_extension_flag : 1;
	unsigned char PES_CRC_flag : 1;
	unsigned char additional_copy_info_flag : 1;
	unsigned char DSM_trick_mode_flag : 1;
	unsigned char ES_rate_flag : 1;
	unsigned char ESCR_flag : 1;
	unsigned char PTS_DTS_flags : 2;

	unsigned char PES_header_data_length;
}optional_pes_header_t;
#pragma pack ()

enum PSStatus
{
	ps_padding, //未知状态
	ps_ps,      //ps状态
	ps_sh,
	ps_psm,
	ps_pes,
	ps_pes_video,
	ps_pes_audio
};

static inline unsigned __int64 ff_parse_pes_pts(const unsigned char* buf) {
	return (unsigned __int64)(*buf & 0x0e) << 29 |
		(AV_RB16(buf + 1) >> 1) << 15 |
		AV_RB16(buf + 3) >> 1;
}

static unsigned __int64 get_pts(optional_pes_header* option)
{
	if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
	{
		return 0;
	}
	if ((option->PTS_DTS_flags & 2) == 2)
	{
		unsigned char* pts = (unsigned char*)option + sizeof(optional_pes_header);
		return ff_parse_pes_pts(pts);
	}
	return 0;
}

static unsigned __int64 get_dts(optional_pes_header* option)
{
	if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
	{
		return 0;
	}
	if ((option->PTS_DTS_flags & 3) == 3)
	{
		unsigned char* dts = (unsigned char*)option + sizeof(optional_pes_header) + 5;
		return ff_parse_pes_pts(dts);
	}
	return 0;
}

bool inline is_ps_header(ps_header_t* ps)
{
	if (ps->pack_start_code[0] == 0 && ps->pack_start_code[1] == 0 && ps->pack_start_code[2] == 1 && ps->pack_start_code[3] == 0xBA)
		return true;
	return false;
}

bool inline is_sh_header(sh_header_t* sh)
{
	if (sh->system_header_start_code[0] == 0 && sh->system_header_start_code[1] == 0 && sh->system_header_start_code[2] == 1 && sh->system_header_start_code[3] == 0xBB)
		return true;
	return false;
}

bool inline is_psm_header(psm_header_t* psm)
{
	if (psm->promgram_stream_map_start_code[0] == 0 && psm->promgram_stream_map_start_code[1] == 0 && psm->promgram_stream_map_start_code[2] == 1 && psm->promgram_stream_map_start_code[3] == 0xBC)
		return true;
	return false;
}

bool inline is_pes_video_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xE0)
		return true;
	return false;
}

bool inline is_pes_audio_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xC0)
		return true;
	return false;
}

bool inline is_pes_header(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1)
	{
		if (pes->stream_id == 0xC0 || pes->stream_id == 0xE0)
		{
			return true;
		}
	}
	return false;
}

PSStatus inline pes_type(pes_header_t* pes)
{
	if (pes->pes_start_code_prefix[0] == 0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1)
	{
		if (pes->stream_id == 0xC0)
		{
			return ps_pes_audio;
		}
		else if (pes->stream_id == 0xE0)
		{
			return ps_pes_video;
		}
	}
	return ps_padding;
}

/*
_1 是否包含数据
_2 下一个PS状态
_3 数据指针
_4 数据长度
*/
typedef tuple<bool, PSStatus, pes_header_t*> pes_tuple;
/*
_1 是否包含数据
_2 数据类型
_3 PTS时间戳
_4 DTS时间戳
_5 数据指针
_6 数据长度
*/
typedef tuple<bool, unsigned char, unsigned __int64, unsigned __int64, char*, unsigned int> naked_tuple;

class PsParser
{
public:
	PsParser()
	{
		m_status = ps_padding;
		m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;
	}
	void PSWrite(char* pBuffer, unsigned int sz)
	{
		if (m_nPSWrtiePos + sz < MAX_PS_LENGTH)
		{
			memcpy((m_pPSBuffer + m_nPSWrtiePos), pBuffer, sz);
			m_nPSWrtiePos += sz;
		}
		else
		{
			m_status = ps_padding;
			m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;
		}

	}

	pes_tuple pes_payload()
	{
		if (m_status == ps_padding)
		{
			for (; m_nPESIndicator < m_nPSWrtiePos; m_nPESIndicator++)
			{
				m_ps = (ps_header_t*)(m_pPSBuffer + m_nPESIndicator);
				if (is_ps_header(m_ps))
				{
					m_status = ps_ps;
					break;
				}
			}
		}
		if (m_status == ps_ps)
		{
			for (; m_nPESIndicator < m_nPSWrtiePos; m_nPESIndicator++)
			{
				m_sh = (sh_header_t*)(m_pPSBuffer + m_nPESIndicator);
				m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);
				if (is_sh_header(m_sh))
				{
					m_status = ps_sh;
					break;
				}
				else if (is_pes_header(m_pes))
				{
					m_status = ps_pes;
					break;
				}
			}
		}
		if (m_status == ps_sh)
		{
			for (; m_nPESIndicator < m_nPSWrtiePos; m_nPESIndicator++)
			{
				m_psm = (psm_header_t*)(m_pPSBuffer + m_nPESIndicator);
				m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);
				if (is_psm_header(m_psm))
				{
					m_status = ps_psm;//冲掉s_sh状态
					break;
				}
				if (is_pes_header(m_pes))
				{
					m_status = ps_pes;
					break;
				}
			}
		}
		if (m_status == ps_psm)
		{
			for (; m_nPESIndicator < m_nPSWrtiePos; m_nPESIndicator++)
			{
				m_pes = (pes_header_t*)(m_pPSBuffer + m_nPESIndicator);
				if (is_pes_header(m_pes))
				{
					m_status = ps_pes;
					break;
				}
			}
		}
		if (m_status == ps_pes)
		{
			//寻找下一个pes 或者 ps
			unsigned short PES_packet_length = ntohs(m_pes->PES_packet_length);
			if ((m_nPESIndicator + PES_packet_length + sizeof(pes_header_t)) < m_nPSWrtiePos)
			{
				char* next = (m_pPSBuffer + m_nPESIndicator + sizeof(pes_header_t) + PES_packet_length);
				pes_header_t* pes = (pes_header_t*)next;
				ps_header_t* ps = (ps_header_t*)next;
				m_nPESLength = PES_packet_length + sizeof(pes_header_t);
				memcpy(m_pPESBuffer, m_pes, m_nPESLength);
				if (is_pes_header(pes) || is_ps_header(ps))
				{
					PSStatus status = ps_padding;
					if (is_ps_header(ps))
					{
						status = m_status = ps_ps;
					}
					else
					{
						status = pes_type(pes);
					}
					int remain = m_nPSWrtiePos - (next - m_pPSBuffer);
					memcpy(m_pPSBuffer, next, remain);
					m_nPSWrtiePos = remain; m_nPESIndicator = 0;
					m_ps = (ps_header_t*)m_pPSBuffer;
					m_pes = (pes_header_t*)m_pPSBuffer;
					return pes_tuple(true, status, (pes_header_t*)m_pPESBuffer);
				}
				else
				{
					m_status = ps_padding;
					m_nPESIndicator = m_nPSWrtiePos = 0;
				}
			}
		}
		return pes_tuple(false, ps_padding, NULL);
	}

	naked_tuple naked_payload()
	{
		naked_tuple tuple = naked_tuple(false, 0, 0, 0, NULL, 0);
		do
		{
			pes_tuple t = pes_payload();
			if (!std::tr1::get<0>(t))
			{
				break;
			}
			PSStatus status = std::tr1::get<1>(t);
			pes_header_t* pes = std::tr1::get<2>(t);
			optional_pes_header* option = (optional_pes_header*)((char*)pes + sizeof(pes_header_t));
			if (option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0)
			{
				break;
			}
			unsigned __int64 pts = get_pts(option);
			unsigned __int64 dts = get_dts(option);
			unsigned char stream_id = pes->stream_id;
			unsigned short PES_packet_length = ntohs(pes->PES_packet_length);
			char* pESBuffer = ((char*)option + sizeof(optional_pes_header) + option->PES_header_data_length);
			int nESLength = PES_packet_length - (sizeof(optional_pes_header) + option->PES_header_data_length);
			memcpy(m_pESBuffer + m_nESLength, pESBuffer, nESLength);
			m_nESLength += nESLength;
			if (stream_id == 0xE0 && (status == ps_ps || status == ps_pes_audio))
			{
				tuple = naked_tuple(true, 0xE0, pts, dts, m_pESBuffer, m_nESLength);
				m_nESLength = 0;
			}
			else if (stream_id == 0xC0)
			{
				tuple = naked_tuple(true, 0xC0, pts, dts, m_pESBuffer, m_nESLength);
				m_nESLength = 0;
			}
		} while (false);
		return tuple;
	}

private:
	PSStatus      m_status;                     //当前状态
	char          m_pPSBuffer[MAX_PS_LENGTH];   //PS缓冲区
	unsigned int  m_nPSWrtiePos;                //PS写入位置
	unsigned int  m_nPESIndicator;              //PES指针
private:
	char          m_pPESBuffer[MAX_PES_LENGTH]; //PES缓冲区
	unsigned int  m_nPESLength;                 //PES数据长度
private:
	ps_header_t*  m_ps;                         //PS头
	sh_header_t*  m_sh;                         //系统头
	psm_header_t* m_psm;                        //节目流头
	pes_header_t* m_pes;                        //PES头
private:
	char         m_pESBuffer[MAX_ES_LENGTH];    //裸码流
	unsigned int m_nESLength;                   //裸码流长度
};

#endif
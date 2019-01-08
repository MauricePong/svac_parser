#include "ps_parser.h"
#include "string.h"
#define TRUE 1
#define FALSE 0

#ifdef WIN32
#include <winsock.h>
#pragma comment(lib,"ws2_32.lib")
#endif

#define MAX_PS_LENGTH  (0xFFFF)
#define MAX_PES_LENGTH (0xFFFF)
#define MAX_ES_LENGTH  (0x100000)
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
#pragma pack (1)
typedef struct RTP_HEADER
{
#ifdef ORTP_BIGENDIAN
    uint16_t version:2;
    uint16_t padbit:1;
    uint16_t extbit:1;
    uint16_t cc:4;
    uint16_t markbit:1;
    uint16_t paytype:7;
#else
    uint16_t cc:4;
    uint16_t extbit:1;
    uint16_t padbit:1;
    uint16_t version:2;
    uint16_t paytype:7;  //负载类型
    uint16_t markbit:1;  //1表示前面的包为一个解码单元,0表示当前解码单元未结束
#endif
    uint16_t seq_number;  //序号
    uint32_t timestamp; //时间戳
    uint32_t ssrc;  //循环校验码
    //uint32_t csrc[16];
} RTP_header_t;

typedef struct ps_header{
    unsigned char pack_start_code[4];  //'0x000001BA'

    unsigned char system_clock_reference_base21:2;
    unsigned char marker_bit:1;
    unsigned char system_clock_reference_base1:3;
    unsigned char fix_bit:2;    //'01'

    unsigned char system_clock_reference_base22;

    unsigned char system_clock_reference_base31:2;
    unsigned char marker_bit1:1;
    unsigned char system_clock_reference_base23:5;

    unsigned char system_clock_reference_base32;
    unsigned char system_clock_reference_extension1:2;
    unsigned char marker_bit2:1;
    unsigned char system_clock_reference_base33:5; //system_clock_reference_base 33bit

    unsigned char marker_bit3:1;
    unsigned char system_clock_reference_extension2:7; //system_clock_reference_extension 9bit

    unsigned char program_mux_rate1;

    unsigned char program_mux_rate2;
    unsigned char marker_bit5:1;
    unsigned char marker_bit4:1;
    unsigned char program_mux_rate3:6;

    unsigned char pack_stuffing_length:3;
    unsigned char reserved:5;
}ps_header_t;  //14

typedef struct sh_header
{
    unsigned char system_header_start_code[4]; //32

    unsigned char header_length[2];            //16 uimsbf

    uint32_t marker_bit1:1;   //1  bslbf
    uint32_t rate_bound:22;   //22 uimsbf
    uint32_t marker_bit2:1;   //1 bslbf
    uint32_t audio_bound:6;   //6 uimsbf
    uint32_t fixed_flag:1;    //1 bslbf
    uint32_t CSPS_flag:1;     //1 bslbf

    uint16_t system_audio_lock_flag:1;  // bslbf
    uint16_t system_video_lock_flag:1;  // bslbf
    uint16_t marker_bit3:1;             // bslbf
    uint16_t video_bound:5;             // uimsbf
    uint16_t packet_rate_restriction_flag:1; //bslbf
    uint16_t reserved_bits:7;                //bslbf
    unsigned char reserved[6];
}sh_header_t; //18

typedef struct psm_header{
    unsigned char promgram_stream_map_start_code[4];

    unsigned char program_stream_map_length[2];

    unsigned char program_stream_map_version:5;
    unsigned char reserved1:2;
    unsigned char current_next_indicator:1;

    unsigned char marker_bit:1;
    unsigned char reserved2:7;

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

typedef struct optional_pes_header_t{
    unsigned char original_or_copy:1;
    unsigned char copyright:1;
    unsigned char data_alignment_indicator:1;
    unsigned char PES_priority:1;
    unsigned char PES_scrambling_control:2;
    unsigned char fix_bit:2;

    unsigned char PES_extension_flag:1;
    unsigned char PES_CRC_flag:1;
    unsigned char additional_copy_info_flag:1;
    unsigned char DSM_trick_mode_flag:1;
    unsigned char ES_rate_flag:1;
    unsigned char ESCR_flag:1;
    unsigned char PTS_DTS_flags:2;

    unsigned char PES_header_data_length;
}optional_pes_header_t;

#pragma pack ()


typedef enum PSStatus{
    ps_padding, //未知状态
    ps_ps,      //ps状态
    ps_sh,
    ps_psm,
    ps_pes,
    ps_pes_video,
    ps_pes_audio
}PSStatus;

#ifndef AV_RB16
#   define AV_RB16(x)                           \
    ((((const unsigned char*)(x))[0] << 8) |          \
    ((const unsigned char*)(x))[1])
#endif


static uint64_t ff_parse_pes_pts(const unsigned char* buf) {
    return (uint64_t)(*buf & 0x0e) << 29 |
        (AV_RB16(buf+1) >> 1) << 15 |
        AV_RB16(buf+3) >> 1;
}

static uint64_t get_pts(optional_pes_header_t* option){
    if(option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0){
        return 0;
    }
    if((option->PTS_DTS_flags & 2) == 2){
        unsigned char* pts = (unsigned char*)option + sizeof(optional_pes_header_t);
        return ff_parse_pes_pts(pts);
    }
    return 0;
}

static uint64_t get_dts(optional_pes_header_t* option){
    if(option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0){
        return 0;
    }
    if((option->PTS_DTS_flags & 3) == 3){
        unsigned char* dts = (unsigned char*)option + sizeof(optional_pes_header_t) + 5;
        return ff_parse_pes_pts(dts);
    }
    return 0;
}
static int is_ps_header(ps_header_t* ps){
    if(ps->pack_start_code[0] == 0 && ps->pack_start_code[1] == 0 && ps->pack_start_code[2] == 1 && ps->pack_start_code[3] == 0xBA)
        return TRUE;
    return FALSE;
}
static int is_sh_header(sh_header_t* sh){
    if(sh->system_header_start_code[0] == 0 && sh->system_header_start_code[1] == 0 && sh->system_header_start_code[2] == 1 && sh->system_header_start_code[3] == 0xBB)
        return TRUE;
    return FALSE;
}
static int is_psm_header(psm_header_t* psm)
{
    if(psm->promgram_stream_map_start_code[0] == 0 && psm->promgram_stream_map_start_code[1] == 0 && psm->promgram_stream_map_start_code[2] == 1 && psm->promgram_stream_map_start_code[3] == 0xBC)
        return TRUE;
    return FALSE;
}
static int is_pes_video_header(pes_header_t* pes)
{
    if(pes->pes_start_code_prefix[0]==0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xE0)
        return TRUE;
    return FALSE;
}
static int is_pes_audio_header(pes_header_t* pes){
    if(pes->pes_start_code_prefix[0]==0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1 && pes->stream_id == 0xC0)
        return TRUE;
    return FALSE;
}
static int is_pes_header(pes_header_t* pes){
    if(pes->pes_start_code_prefix[0]==0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1){
        if(pes->stream_id == 0xC0 || pes->stream_id == 0xE0){
            return TRUE;
        }
    }
    return FALSE;
}

int is_private_stream_header(pes_header_t* pes){
    if(pes->pes_start_code_prefix[0]==0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1){
        if(pes->stream_id == 0xBD || pes->stream_id == 0xBF){
            return TRUE;
        }
    }
    return FALSE;
}

PSStatus pes_type(pes_header_t* pes){
    if(pes->pes_start_code_prefix[0]==0 && pes->pes_start_code_prefix[1] == 0 && pes->pes_start_code_prefix[2] == 1){
        if(pes->stream_id == 0xC0){
            return ps_pes_audio;
        }
        else if(pes->stream_id == 0xE0){
            return ps_pes_video;
        }
    }
    return ps_padding;
}
/*
_1 是否包含数据
_2 下一个PS状态
_3 数据指针
*/
typedef struct pes_tuple_t{
	int has_data;
	PSStatus status;
	pes_header_t* hdr;
}pes_tuple_t;
static pes_tuple_t pes_tuple(int has_data, PSStatus status, pes_header_t* hdr){
	pes_tuple_t t={has_data,status,hdr};
	return t;
}
/*
_1 是否包含数据
_2 数据类型
_3 PTS时间戳
_4 DTS时间戳
_5 数据指针
_6 数据长度
*/
typedef struct naked_tuple_t{
	int has_data;
	unsigned char type;
	uint64_t pts;
	uint64_t dts;
	char* data;
	unsigned int len;
}naked_tuple_t;
//typedef std::tr1::tuple<bool, unsigned char, unsigned __int64, unsigned __int64, char*, unsigned int> naked_tuple;
static naked_tuple_t naked_tuple(int has_data, unsigned char type, uint64_t pts , uint64_t dts, char* data, unsigned int len){
	naked_tuple_t t={has_data,type,pts,dts,data,len};
	return t;
}

typedef struct ps_packet_t{
	PSStatus      m_status;                     //当前状态
    char          m_pPSBuffer[MAX_PS_LENGTH];   //PS缓冲区
    unsigned int  m_nPSWrtiePos;                //PS写入位置
    unsigned int  m_nPESIndicator;              //PES指针
    char          m_pPESBuffer[MAX_PES_LENGTH]; //PES缓冲区
    unsigned int  m_nPESLength;                 //PES数据长度
 	ps_header_t*  m_ps;                         //PS头
 	sh_header_t*  m_sh;                         //系统头
 	psm_header_t* m_psm;                        //节目流头
 	pes_header_t* m_pes;                        //PES头
    char         m_pESBuffer[MAX_ES_LENGTH];    //裸码流
    unsigned int m_nESLength;                   //裸码流长度
}ps_packet_t;

//新建立
static ps_packet_t* ps_packet_new(){
	ps_packet_t* p=new ps_packet_t;
	p->m_status = ps_padding;
    p->m_nESLength = p->m_nPESIndicator = p->m_nPSWrtiePos = p->m_nPESLength = 0;
	return p;
}
//destroy
static void ps_packet_destroy(ps_packet_t* p){
	delete p;
}
//ps_write
static void ps_write(ps_packet_t* p,char* pBuffer, unsigned int sz){
    if(p->m_nPSWrtiePos + sz < MAX_PS_LENGTH){
        memcpy((p->m_pPSBuffer + p->m_nPSWrtiePos), pBuffer, sz);
        p->m_nPSWrtiePos += sz;
    }
    else
    {
        p->m_status = ps_padding;
        p->m_nESLength = p->m_nPESIndicator = p->m_nPSWrtiePos = p->m_nPESLength = 0;
    }
}
//ps_rtpwrite
static void ps_rtpwrite(ps_packet_t* p,char* pBuffer, unsigned int sz){
    char* data = (pBuffer + sizeof(RTP_header_t));
    unsigned int length =  sz - sizeof(RTP_header_t);
    if(p->m_nPSWrtiePos + length < MAX_PS_LENGTH){
        memcpy((p->m_pPSBuffer + p->m_nPSWrtiePos), data, length);
        p->m_nPSWrtiePos += length;
    }
    else{
        p->m_status = ps_padding;
        p->m_nESLength = p->m_nPESIndicator = p->m_nPSWrtiePos = p->m_nPESLength = 0;
    }
}

static pes_tuple_t pes_payload(ps_packet_t* p){
    if(p->m_status == ps_padding){
        for(; p->m_nPESIndicator<p->m_nPSWrtiePos; p->m_nPESIndicator++){
            p->m_ps = (ps_header_t*)(p->m_pPSBuffer + p->m_nPESIndicator);
            if(is_ps_header(p->m_ps)){
                p->m_status = ps_ps;
                break;
            }
        }
    }
    if(p->m_status == ps_ps){
        for(; p->m_nPESIndicator<p->m_nPSWrtiePos; p->m_nPESIndicator++){
            p->m_sh = (sh_header_t*)(p->m_pPSBuffer + p->m_nPESIndicator);
            p->m_pes = (pes_header_t*)(p->m_pPSBuffer + p->m_nPESIndicator);
            if(is_sh_header(p->m_sh)){
                p->m_status = ps_sh;
                break;
            }
            else if (is_pes_header(p->m_pes)){
                p->m_status = ps_pes;
                break;
            }
        }
    }
    if(p->m_status == ps_sh){
        for(; p->m_nPESIndicator<p->m_nPSWrtiePos; p->m_nPESIndicator++){
            p->m_psm = (psm_header_t*)(p->m_pPSBuffer + p->m_nPESIndicator);
            p->m_pes = (pes_header_t*)(p->m_pPSBuffer + p->m_nPESIndicator);
            if(is_psm_header(p->m_psm)){
                p->m_status = ps_psm;//冲掉s_sh状态
                break;
            }
            if(is_pes_header(p->m_pes)){
                p->m_status = ps_pes;
                break;
            }
        }
    }
    if(p->m_status == ps_psm){
        for(; p->m_nPESIndicator<p->m_nPSWrtiePos; p->m_nPESIndicator++){
            p->m_pes = (pes_header_t*)(p->m_pPSBuffer + p->m_nPESIndicator);
            if(is_pes_header(p->m_pes)){
                p->m_status = ps_pes;
                break;
            }
        }
    }
    if(p->m_status == ps_pes){
        //寻找下一个pes 或者 ps
        unsigned short PES_packet_length = ntohs(p->m_pes->PES_packet_length);
        if((p->m_nPESIndicator + PES_packet_length + sizeof(pes_header_t)) < p->m_nPSWrtiePos){
            char* next = (p->m_pPSBuffer + p->m_nPESIndicator + sizeof(pes_header_t) + PES_packet_length);
            pes_header_t* pes = (pes_header_t*)next;
            ps_header_t* ps = (ps_header_t*)next;
            p->m_nPESLength = PES_packet_length + sizeof(pes_header_t);
            memcpy(p->m_pPESBuffer, p->m_pes, p->m_nPESLength);
            if( is_private_stream_header(pes) || is_pes_header(pes) || is_ps_header(ps)){
                PSStatus status = ps_padding;
				int remain = p->m_nPSWrtiePos - (next - p->m_pPSBuffer);
                if(is_ps_header(ps)){
                    status = p->m_status = ps_ps;
                }
                else{
                    status = pes_type(pes);
                }
                memcpy(p->m_pPSBuffer, next, remain);
                p->m_nPSWrtiePos = remain; p->m_nPESIndicator = 0;
                p->m_ps = (ps_header_t*)p->m_pPSBuffer;
                p->m_pes = (pes_header_t*)p->m_pPSBuffer;
                return pes_tuple(TRUE, status, (pes_header_t*)p->m_pPESBuffer);
            }
            else{
                p->m_status = ps_padding;
                p->m_nPESIndicator = p->m_nPSWrtiePos = 0;
            }
        }
    }
    return pes_tuple(FALSE, ps_padding, NULL);
}
static naked_tuple_t naked_payload(ps_packet_t* p){
    naked_tuple_t tuple = naked_tuple(FALSE, 0, 0, 0, NULL, 0);
    do{
        pes_tuple_t t = pes_payload(p);
		if(! t.has_data)break;
		else{
			PSStatus status = t.status;
			pes_header_t* pes = t.hdr;
			optional_pes_header_t* option = (optional_pes_header_t*)((char*)pes + sizeof(pes_header_t));
			uint64_t pts = get_pts(option);
			uint64_t dts = get_dts(option);
			unsigned char stream_id = pes->stream_id;
			unsigned short PES_packet_length = ntohs(pes->PES_packet_length);
			char* pESBuffer = ((char*)option + sizeof(optional_pes_header_t) + option->PES_header_data_length);
			int nESLength = PES_packet_length - (sizeof(optional_pes_header_t) + option->PES_header_data_length);
			if(option->PTS_DTS_flags != 2 && option->PTS_DTS_flags != 3 && option->PTS_DTS_flags != 0){
				break;
			}
			memcpy(p->m_pESBuffer + p->m_nESLength, pESBuffer, nESLength);
			p->m_nESLength += nESLength;
			if(stream_id == 0xE0 && (status == ps_ps || status == ps_pes_audio)){
				tuple = naked_tuple(TRUE, 0xE0, pts, dts, p->m_pESBuffer, p->m_nESLength);
				p->m_nESLength = 0;
			}
			else if(stream_id == 0xC0){
				tuple = naked_tuple(TRUE, 0xC0, pts, dts, p->m_pESBuffer, p->m_nESLength);
				p->m_nESLength = 0;
			}
		}
    } while (FALSE);
    return tuple;
}

typedef struct ps_parser_t{
	//parser
	ps_packet_t* 	packet;
	//call back
	ps_packet_cb	cb;
	//user data
	void* ud;
}ps_parser_t;
//new
ps_parser_t* ps_parser_new(void* ud,ps_packet_cb cb){
	ps_parser_t* ps = new ps_parser_t;
	ps->packet=ps_packet_new();
	ps->ud=ud;
	ps->cb=cb;
	return ps;
}
//destroy
void ps_parser_destroy(ps_parser_t* ps){
	if(ps->packet)ps_packet_destroy(ps->packet);
	delete ps;
}

//get data
int ps_parser_process(ps_parser_t* ps, unsigned char *pu8PSData, unsigned int u32PSDataLength){
	naked_tuple_t t = { 0 };
	ps_write(ps->packet, (char*)pu8PSData, u32PSDataLength);
	t = naked_payload(ps->packet);
	if (t.has_data&&ps->cb){
		ps->cb(t.type, t.pts, t.dts, (uint8_t*)t.data, t.len,ps->ud);
		return t.len;
	}
	return 0;
}



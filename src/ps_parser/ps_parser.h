#ifndef __PS__PARSER__INC__HH__
#define __PS__PARSER__INC__HH__

#ifdef _MSC_VER
#if defined(MEDIA_SDK_EXPORTS)
#define MEDIA_SDK_PUBLIC	__declspec(dllexport)
#else
#define MEDIA_SDK_PUBLIC
#endif
#else
#define MEDIA_SDK_PUBLIC
#endif


#include "stdint.h"
//ps packet
struct ps_parser_t;
typedef struct ps_parser_t ps_parser_t;

//type==1 audio,type==2 video
#define IS_PS_AUDIO(type) type==0xC0
#define IS_PS_VIDEO(type) type==0xE0
//数据回调
typedef void(*ps_packet_cb)(uint64_t type,uint64_t pts, uint64_t dts, uint8_t* data, int len, void* ud);
//new
MEDIA_SDK_PUBLIC ps_parser_t* ps_parser_new(void* ud, ps_packet_cb cb);
//destroy
MEDIA_SDK_PUBLIC void ps_parser_destroy(ps_parser_t* ps);
//get data
MEDIA_SDK_PUBLIC int ps_parser_process(ps_parser_t* ps, unsigned char *pu8PSData, unsigned int u32PSDataLength);

#endif 
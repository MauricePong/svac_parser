#ifndef __PS__PARSER__INC__HH__
#define __PS__PARSER__INC__HH__

#include "head.h"
typedef struct {
	int PES_packet_length;
	int PES_header_data_length;
	unsigned char stream_id;
	unsigned char PTS_DTS_flags;
	unsigned char ESCR_flag;
	unsigned char ES_rate_flag ;
	unsigned char DSM_trick_mode_flag;
	unsigned char additional_copy_info_flag;
	unsigned char PES_CRC_flag;
	unsigned char PES_extension_flag;
	unsigned char PES_private_data_flag;
	unsigned char pack_header_field_flag;
	unsigned char program_packet_sequence_counter_flag;
	unsigned char PSTD_buffer_flag;
	unsigned char PES_extension_flag_2;
	unsigned char original_stuff_length;
	unsigned char PES_extension_field_length;
}PSContext;


class PsParser {
public:
	PsParser();
	~PsParser();
	int jump(FILE *avfp);
	int getEs(FILE *avfp,unsigned char *esbuf,int &len);
	unsigned char buf[1024];
	unsigned char *psbuf;
	int pos;
private:
	int readNext(FILE *avfp);
	PSContext ctx;

};




#endif
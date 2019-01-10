#include "ps_parser.h"
PsParser::PsParser()
{
	cout << "ps parser start.." << endl;
	psbuf = buf;
	pos = 0;
}

PsParser::~PsParser()
{
	cout << "ps parser end.." << endl;
}

int PsParser::jump(FILE *avfp)
{
	int ret = 0;
	psbuf += 4;pos += 4;
	if ((ret = readNext(avfp)) < 0) {
		return ret;
	}
	while (!(psbuf[0] == 0
		&& psbuf[1] == 0
		&& psbuf[2] == 1
		&& (psbuf[3] >= 0xb8
			|| psbuf[3] == 0xb1)))
	{
		psbuf++;pos++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}
	}
	return ret;
}

int PsParser::getEs(FILE *avfp,unsigned char * esbuf, int &len)
{
	int ret = 0;
	int i = 0;
	int j = 0;
	int choicenum = 0;
	ctx.original_stuff_length = 0;
	int n = 0;
	psbuf += 3;
	pos += 3;
	if ((ret = readNext(avfp)) < 0) {
		return ret;
	}

	ctx.stream_id = psbuf[0];
	ctx.PES_packet_length = (psbuf[1] << 8) + psbuf[2];
	psbuf += 3;
	pos += 3;
	if ((ret = readNext(avfp)) < 0) {
		return ret;
	}

	ctx.PTS_DTS_flags = (psbuf[1] >> 6) & 0x03;
	ctx.ESCR_flag = psbuf[1] & (0x01 << 5);
	ctx.ES_rate_flag = psbuf[1] & (0x01 << 4);
	ctx.DSM_trick_mode_flag = psbuf[1] & (0x01 << 3);
	ctx.additional_copy_info_flag = psbuf[1] & (0x01 << 2);
	ctx.PES_CRC_flag = psbuf[1] & (0x01 << 1);
	ctx.PES_extension_flag = psbuf[1] & 0x01;
	ctx.PES_header_data_length = psbuf[2];
	psbuf += 3;
	pos += 3;
	n += 3;
	if ((ret = readNext(avfp)) < 0) {
		return ret;
	}
	//printf("%0x\t%0x\t%0x\n",psbuf[0],psbuf[1],psbuf[2]);
	if ((ctx.PTS_DTS_flags == 0x02)|| (ctx.PTS_DTS_flags == 0x03))
	{
		for (i = 0; i < (ctx.PTS_DTS_flags -0x01)*5; i++)
		{
			psbuf++;pos++;n++;choicenum++;
			if ((ret = readNext(avfp)) < 0) {
				return ret;
			}
		}
	}

	if (ctx.ESCR_flag != 0)
	{
		for (i = 0; i < 6; i++)
		{
			psbuf++; pos++; n++; choicenum++;
			if ((ret = readNext(avfp)) < 0) {
				return ret;
			}
		}
	}

	if (ctx.ES_rate_flag != 0)
	{
		for (i = 0; i < 3; i++)
		{
			psbuf++; pos++; n++; choicenum++;
			if ((ret = readNext(avfp)) < 0) {
				return ret;
			}
		}
	}

	if (ctx.DSM_trick_mode_flag != 0)
	{
		psbuf++; pos++; n++; choicenum++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}
	}

	if (ctx.additional_copy_info_flag != 0)
	{
		psbuf++; pos++; n++; choicenum++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}
	}

	if (ctx.PES_CRC_flag != 0)
	{
		psbuf += 2; pos += 2; n++; choicenum++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}
	}

	if (ctx.PES_extension_flag != 0)
	{
		ctx.PES_private_data_flag = psbuf[0] & (0x01 << 7);
		ctx.pack_header_field_flag = psbuf[0] & (0x01 << 6);
		ctx.program_packet_sequence_counter_flag = psbuf[0] & (0x01 << 5);
		ctx.PSTD_buffer_flag = psbuf[0] & (0x01 << 4);
		ctx.PES_extension_flag_2 = psbuf[0] & 0x01;
		psbuf++; pos++; n++; choicenum++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}

		if (ctx.PES_private_data_flag != 0)
		{
			for (i = 0; i < 16; i++)
			{
				psbuf++; pos++; n++; choicenum++;
				if ((ret = readNext(avfp)) < 0) {
					return ret;
				}
			}
		}

		if (ctx.pack_header_field_flag != 0)
		{
			psbuf++; pos++; n++; choicenum++;
			if ((ret = readNext(avfp)) < 0) {
				return ret;
			}
			return ret;
		}

		if (ctx.program_packet_sequence_counter_flag != 0)
		{
			ctx.original_stuff_length = psbuf[1] & 0x3f;
			psbuf += 2; pos += 2; n += 2; choicenum += 2;
			if ((ret = readNext(avfp)) < 0) {
				return ret;
			}
		}

		if (ctx.PSTD_buffer_flag != 0)
		{
			psbuf += 2; pos += 2; n += 2; choicenum += 2;
			ret = readNext(avfp);
			if (ret < 0) {
				return ret;
			}
		}

		if (ctx.PES_extension_flag_2 != 0)
		{
			ctx.PES_extension_field_length = psbuf[0] & 0x7f;
			psbuf++; pos++; n++; choicenum++;
			if ((ret = readNext(avfp)) < 0) {
				return ret;
			}
			for (i = 0; i < ctx.PES_extension_field_length; i++)
			{
				psbuf++; pos++; n++; choicenum++;
				if ((ret = readNext(avfp)) < 0) {
					return ret;
				}
			}
		}
	}

	j = ctx.PES_header_data_length - choicenum;
	for (i = 0; i < j; i++)
	{
		psbuf++; pos++; n++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}
	}

	len = ctx.PES_packet_length - n;
	for (i = 0; i < len; i++)
	{
		//fwrite(psbuf, 1, 1, fp);
		esbuf[i] = *psbuf;
		psbuf++; pos++;
		if ((ret = readNext(avfp)) < 0) {
			return ret;
		}
	}
	return ret;
}

int PsParser::readNext(FILE *avfp)
{
	if ((pos < 1021)||(pos >1024)) {
		return 0;
	}
	for (int i = 0; i < (1024 - pos); i++) {
		buf[i] = psbuf[i];
	}
	if (fread(buf + (1024 - pos), 1, pos, avfp) <= 0)
	{
		return -1;
	}
	psbuf = buf;
	pos = 0;
	return 0;
}



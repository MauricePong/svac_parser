#include "ps_parser.h"
PsParser::PsParser()
{
	cout << "ps parser start.." << endl;
	m_status = ps_padding;
	m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;
}

PsParser::~PsParser()
{
	cout << "ps parser end.." << endl;
}

void PsParser::PSWrite(char * pBuffer, unsigned int sz)
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

void PsParser::RTPWrite(char * pBuffer, unsigned int sz)
{
	char* data = (pBuffer + sizeof(RTP_header_t));
	unsigned int length = sz - sizeof(RTP_header_t);
	if (m_nPSWrtiePos + length < MAX_PS_LENGTH)
	{
		memcpy((m_pPSBuffer + m_nPSWrtiePos), data, length);
		m_nPSWrtiePos += length;
	}
	else
	{
		m_status = ps_padding;
		m_nESLength = m_nPESIndicator = m_nPSWrtiePos = m_nPESLength = 0;
	}

}

pes_tuple PsParser::pes_payload()
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

naked_tuple PsParser::naked_payload()
{
	naked_tuple tuple = naked_tuple(false, 0, 0, 0, NULL, 0);
	do
	{
		pes_tuple t = pes_payload();
		if (!get<0>(t))
		{
			break;
		}
		PSStatus status = get<1>(t);
		pes_header_t* pes = get<2>(t);
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

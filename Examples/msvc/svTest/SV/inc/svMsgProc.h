#pragma once
#include <string.h>
#include "scanviz.h"
#include "streambuf.h"

#define SERBUFSIZE 65536U
#define MSGBUFSIZE 256

class SvMsgProc {
public:

	CStreamBuffer <SERBUFSIZE> msgStream; /* ��������� ����� ��� ������ � ������� */

public:
	SvMsgProc();
	~SvMsgProc();
	scanvizHdr_t* Get_Msg();
	void Free_Msg();
	size_t Build_Msg(void* pMsg, uint8_t msgID, uint16_t bodylength);
	/**
	 * @brief ��������� ��������� �������/���������, ������������ ����������� �����.
	 * ������������ ��� ������/��������� ������������� �����.
	 * @param T ��� ������������� ���������/�������. ������ ���� ����� �� �����
	 * �������� ������/���������, ����������� � scanviz.h
	 * @param id �������������� �������/���������. ������ ��������������� ���� ���������.
	 * @returns ����� ��������������� ���������
	 */
	template < typename T, uint8_t id >
		size_t Build_Msg2(T* msg) {
			/* ��������� ��������� */
			scanvizHdr_t* hdr = (scanvizHdr_t*)msg;
			hdr->sync = SCANVIZ_START_MARKER;
			hdr->msgID = id;
			hdr->dlen = sizeof(T) - sizeof(scanvizHdr_t) - 2;
			hdr->ver = SCANVIZ_VERSION;
			hdr->resv = 0;
			
			/* ������� ����������� �����  */
			uint16_t* p16 = (uint16_t*)msg;
			uint16_t chksum = 0;
			int32_t cnt = sizeof(T) - 4;
			while (cnt > 0) {
				chksum ^= *(++p16);
				cnt -= 2;
			}
			*(++p16) = chksum;
			return sizeof(T);
		}
	
private:
	enum _state { WAIT_START = 0, WAIT_SIZE, WAIT_DATA, CHECK, COMPLETE, LOCKED }state;
	uint8_t indx;
	uint8_t msgBuf[MSGBUFSIZE]{};		/* �������� ��������� ��� ������� */
	uint32_t nErrors = 0;				/* ������� ������ */
	static const uint16_t maxDataSize = sizeof(msgBuf) - sizeof(scanvizHdr_t) - 2;
};
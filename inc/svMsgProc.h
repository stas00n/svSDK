#pragma once
#include <string.h>
#include "scanviz.h"
#include "streambuf.h"

#define STREAMBUFSIZE 8192U
#define MSGBUFSIZE 256

class SvMsgProc {
public:
	alignas(8) uint8_t msgBuf[MSGBUFSIZE]{};	/* �������� ��������� ��� ������� */
	CStreamBuffer <STREAMBUFSIZE> msgStream;	/* ��������� ����� ��� ������ */

public:
	SvMsgProc();
	~SvMsgProc();

	/**
	 * @brief ����� ���������/������� � ������
	 * @returns ��� ��������� ��������� ���������� ��������� �� ��������� (����������
	 * ��������� �� ���� msgBuf[], ��� ���������� ��� ���������). ���� ��������� ��� - 
	 * ���������� nullptr.
	 * ����� ��������� ��������� ����������� ������� Free_Msg() ��� ������������� ������!
	 */
	scanvizHdr_t* Get_Msg();


	/**
	 * @brief ������������� ������ ���������. �������� msgBuf[]. ���������� �������� ������ ���
	 * ����� (Get_Msg() != nullptr)
	 */
	void Free_Msg();


	/**
	 * @brief ��������� ��������� �������/���������, ������������ ����������� �����.
	 * ������������ ��� ������/��������� �������������/���������� �����.
	 * @param pMsg ��������� �� ��������� ���������/�������. 
	 * @param msgID ������������� �������/���������. ������ ��������������� ���� ���������.
	 * @param bodylength ����� ���� ���������. ������ ���� ������! ��� ��������� ������������� ����� ����� 
	 * ������������ ������ BODYLENGTH(type). ��� ������ (���������� �����) - strlen + ����������
	 * �� ��������.
	 * @returns ����� �������� ���������.
	 */
	uint16_t Build_Msg(void *pMsg, svID_t msgID, uint16_t bodylength);



	/**
	 * @brief ��������� ��������� �������/���������, ������������ ����������� �����.
	 * ������������ ��� ������/��������� ������������� �����.
	 * @param T ��� ������������� ���������/�������. ������ ���� ����� �� �����
	 * �������� ������/���������, ����������� � scanviz.h
	 * @param id �������������� �������/���������. ������ ��������������� ���� ���������.
	 * @returns ����� ��������������� ���������
	 */
	template < typename T, uint8_t id >
	uint16_t Build_Msg2(T* msg) {
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
	enum class _state { WAIT_START = 0, WAIT_SIZE, WAIT_DATA, CHECK, COMPLETE, LOCKED }state;
	uint8_t indx = 0;
	uint32_t nErrors = 0;				/* ������� ������ */
	static const uint16_t maxDataSize = sizeof(msgBuf) - sizeof(scanvizHdr_t) - 2;
};

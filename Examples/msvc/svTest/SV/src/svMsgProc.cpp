#include "scanviz.h"
#include "svMsgProc.h"

SvMsgProc::SvMsgProc() {
	state = WAIT_START;
	indx = 0;
}

SvMsgProc::~SvMsgProc() {
	;
}



size_t SvMsgProc::Build_Msg(void* pMsg, uint8_t msgID, uint16_t bodylength) {
	/* Fill header */
	scanvizHdr_t* hdr = (scanvizHdr_t*)pMsg;
	hdr->sync = SCANVIZ_START_MARKER;
	hdr->msgID = msgID;
	hdr->dlen = bodylength;
	hdr->ver = SCANVIZ_VERSION;
	hdr->resv = 0;

	/* Calc chksum  */
	uint16_t* p16 = (uint16_t*)pMsg;
	uint16_t chksum = 0;
	int32_t cnt = bodylength + 6;
	while (cnt > 0) {
		chksum ^= *(++p16);
		cnt -= 2;
	}
	*(++p16) = chksum;
	return (bodylength + 10);
}



void SvMsgProc::Free_Msg() {
	memset(msgBuf, 0, sizeof(msgBuf));
	state = WAIT_START;
	indx = 0;
}


/*

*/
scanvizHdr_t* SvMsgProc::Get_Msg() {

	static uint16_t rem = 0;      /*remainng bytes to complete message */
	scanvizHdr_t* hdr = (scanvizHdr_t*)msgBuf;
	uint16_t cnt;	/* Количество байт доступных для чтения из буфера */
	scanvizHdr_t* retVal = 0;

	static union {
		uint16_t value16;
		struct {
			uint8_t       low;
			uint8_t       high;
		};
	}preamble;

	if (state == LOCKED) {
		return 0;
	}

	if (state == WAIT_START) {

		/* Start Recording */
		cnt = msgStream.GetCount();

		while (cnt) {
			preamble.value16 >>= 8;
			msgStream.Read2(&preamble.high, 1);
			if (preamble.value16 == SCANVIZ_START_MARKER) {
				state = WAIT_SIZE;
				hdr->sync = preamble.value16;
				indx = 2;
				break;
			}
			cnt--;
		}
	}

	if (state == WAIT_SIZE) {
		cnt = msgStream.GetCount();

		while (cnt--) {
			msgStream.Read2(&msgBuf[indx++], 1);
			if (indx == 8) {
				break;
			}
		}

		if (indx == 8) {
			rem = hdr->dlen + 2;
			if (rem > (maxDataSize + 2)) {
				indx = 0;
				state = WAIT_START;
				preamble.value16 = 0;
			}
			else {
				state = WAIT_DATA;
			}
		}
	}

	if (state == WAIT_DATA) {
		cnt = msgStream.GetCount();
		if (cnt > rem) {
			cnt = rem;
		}

		msgStream.Read2(&msgBuf[indx], cnt);
		rem -= cnt;
		indx += cnt;

		if (rem == 0) {
			state = CHECK;
		}

	}

	if (state == CHECK) {
		uint16_t dataLength;
		dataLength = hdr->dlen;
		if (dataLength <= maxDataSize) {
			uint16_t chkCalc = 0, chkRcv;

			/* Calculate chksum  */
			uint16_t* p16 = (uint16_t*)msgBuf;
			int32_t i = dataLength + sizeof(scanvizHdr_t) - 2;
			while (i > 0) {
				chkCalc ^= *(++p16);
				i -= 2;
			}

			/* Find Chksum in message */
			chkRcv = (msgBuf[dataLength + sizeof(scanvizHdr_t) + 1] << 8) + msgBuf[dataLength + sizeof(scanvizHdr_t)];

			/* Compare */
			if (chkCalc == chkRcv) {
				state = COMPLETE;
				retVal = hdr;
			}
			else {
				nErrors++;
				indx = 0;
				state = WAIT_START;
				preamble.value16 = 0;
			}
		}
		else {
			indx = 0;
			state = WAIT_START;
			preamble.value16 = 0;
		}

	}
	if (state == COMPLETE) {
		state = LOCKED;
		indx = 0;
		retVal = (scanvizHdr_t*)msgBuf;
	}

	return retVal;
}





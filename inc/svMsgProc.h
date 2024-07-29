#pragma once
#include <string.h>
#include "scanviz.h"
#include "streambuf.h"

#define STREAMBUFSIZE 8192U
#define MSGBUFSIZE 256

class SvMsgProc {
public:
	alignas(8) uint8_t msgBuf[MSGBUFSIZE]{};	/* ѕрин€тое сообщение или команда */
	CStreamBuffer <STREAMBUFSIZE> msgStream;	/*  ольцевой буфер дл€ приема */

public:
	SvMsgProc();
	~SvMsgProc();

	/**
	 * @brief ѕоиск сообщени€/команды в потоке
	 * @returns ѕри получении сообщени€ возвращает указатель на заголовок (фактически
	 * указатель на член msgBuf[], где содержитс€ все сообщение). ≈сли сообщени€ нет - 
	 * возвращает nullptr.
	 * ѕосле обработки сообщени€ об€зательно вызвать Free_Msg() дл€ разблокировки поиска!
	 */
	scanvizHdr_t* Get_Msg();


	/**
	 * @brief –азблокировка поиска сообщени€. ќбнул€ет msgBuf[]. Ќеобходимо вызывать каждый раз
	 * после (Get_Msg() != nullptr)
	 */
	void Free_Msg();


	/**
	 * @brief «аполн€ет заголовок команды/сообщени€, рассчитывает контрольную сумму.
	 * »спользовать дл€ команд/сообщений фиксированной/переменной длины.
	 * @param pMsg указатель на структуру сообщени€/команды. 
	 * @param msgID идентификатор команды/сообщени€. ƒолжен соответствовать типу сообщени€.
	 * @param bodylength длина тела сообщени€. ƒолжна быть четной! ƒл€ сообщений фиксированной длины можно 
	 * использовать макрос BODYLENGTH(type). ƒл€ текста (переменна€ длина) - strlen + дополнение
	 * до четности.
	 * @returns длина готового сообщени€.
	 */
	uint16_t Build_Msg(void *pMsg, svID_t msgID, uint16_t bodylength);



	/**
	 * @brief «аполн€ет заголовок команды/сообщени€, рассчитывает контрольную сумму.
	 * »спользовать дл€ команд/сообщений фиксированной длины.
	 * @param T тип передаваемого сообщени€/команды. ƒолжен быть одним из типов
	 * структур команд/сообщений, объ€вленных в scanviz.h
	 * @param id идентиффикатор команды/сообщени€. ƒолжен соответствовать типу сообщени€.
	 * @returns длина подготовленного сообщени€
	 */
	template < typename T, uint8_t id >
	uint16_t Build_Msg2(T* msg) {
		/* заполн€ем заголовок */
		scanvizHdr_t* hdr = (scanvizHdr_t*)msg;
		hdr->sync = SCANVIZ_START_MARKER;
		hdr->msgID = id;
		hdr->dlen = sizeof(T) - sizeof(scanvizHdr_t) - 2;
		hdr->ver = SCANVIZ_VERSION;
		hdr->resv = 0;

		/* рассчет контрольной суммы  */
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
	uint32_t nErrors = 0;				/* —четчик ошибок */
	static const uint16_t maxDataSize = sizeof(msgBuf) - sizeof(scanvizHdr_t) - 2;
};

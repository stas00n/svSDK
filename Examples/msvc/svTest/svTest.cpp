
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <string.h>

#include "scanviz.h"
#include "svMsgProc.h"
#include "comport.h"


HANDLE ghEventExit; /* событие для завершения потоков */

DWORD WINAPI threadRx(LPVOID lpParams);
DWORD WINAPI threadReadFromFile(LPVOID lpParams);
DWORD WINAPI threadParse(LPVOID lpParams);
DWORD WINAPI threadWriteFile(LPVOID lpParams);

uint32_t outEnabled = 1; /* вывод в консоль вкл/выкл */ 

CStreamBuffer <262144> sbRawdata;	/* Буфер 256к для записи в файл */
ComPort com;	/* Обертка для работы с COM портом */
SvMsgProc sv;	/* Для работы с командами/логами scanviz */

/**
 * @brief Шаблон отправки команд
 * @param T scanviz команда (структура из scanviz.h) для отправки 
 * (с заполненными полями тела)
 * @param id ID команды. Должен соответствовать типу команды
 * @returns true в случае успешной отправки
 */
template<typename T, uint8_t id>
bool Send_Cmd(T* pMsg);

void PrintHelp();

/**
 * Print double with .3 precision
 */
static char * PrintDbl(double in) {
	static char str[32];
	sprintf_s(str, "%.3lf", in);
	return str;
}

int main(int argc, char** argv) {


	HANDLE hThreadRx = nullptr;
	HANDLE hThreadParse = nullptr;
	HANDLE hThreadReadFromFile = nullptr;
	HANDLE hThreadWriteFile = nullptr;

	if (argc != 3) {
		std::cout << " Error syntax! Usage:\n\tsvTest comportnumber baudrate\n";
		std::cout << "\tsvTest -f filename\n Anykey for exit.\n";
		while (_getch() == 0) {
			Sleep(60);
		}
		return 0;
	}

	ghEventExit = CreateEvent(nullptr, TRUE, FALSE, nullptr);

	if (memcmp(argv[1], "-f", 3) == 0) {
		/* Работаем с файлом */
		hThreadParse = CreateThread(nullptr, 0, threadParse, nullptr, 0, nullptr);
		hThreadReadFromFile = CreateThread(nullptr, 0, threadReadFromFile, argv[2], 0, nullptr);
	}
	else {
		/* Работаем с COM портом */
		uint32_t baud = 0;
		uint32_t comNum = 0;
		sscanf_s(argv[1], "%d", &comNum);
		sscanf_s(argv[2], "%d", &baud);
		if (!com.Open((uint8_t)comNum, baud)) {
			/* Проблемы с портом - выход */
			std::cout << " Error open " << com.szComPortName << "\n " << com.GetLastErrorMessage() << " Anykey for exit.\n";
			while (_getch() == 0) {
				Sleep(60);
			}
			return 0;
		}
		else {
			/* COM OK */
			std::cout << " " << com.szComPortName << " open OK.\n";
			PrintHelp();
			/* Запускаем нужные потоки */
			hThreadParse = CreateThread(nullptr, 0, threadParse, nullptr, 0, nullptr);
			hThreadRx = CreateThread(nullptr, 0, threadRx, nullptr, 0, nullptr);
			if (hThreadRx) {
				SetThreadPriority(hThreadRx, THREAD_PRIORITY_TIME_CRITICAL);
			}
			hThreadWriteFile = CreateThread(nullptr, 0, threadWriteFile, nullptr, 0, nullptr);
		}
	}
	
	while (1) {
		Sleep(60);
		int ch = _getch();
		// Выход
		if ((ch == 'q') || (ch == 'Q')) {
			break;
		}
		// Вкл/выкл вывод в консоль
		if ((ch == 'o') || (ch == 'O')) {
			outEnabled ^= 1;
			if (outEnabled) {
				std::cout << " Output enabled\n";
			}
			else {
				std::cout << " Output disabled\n"; ;
			}
		}
		// хелп
		if ((ch == 'h') || (ch == 'H')) {
			PrintHelp();
		}
		// Примеры отправки команд
		if (ch == '1') {
			/* Создаем команду и заполняем тело команды (все поля, кроме hdr и chksum) */
			cmdCamTrigDist_t cmd;				// Команда триггера по расстоянию
			cmd.distSrc = DISTANCE_SRC_INSPVA;	// триггер от ИНС
			cmd.maxRate = 15;					// 15 fps max
			cmd.trgPulse_ms = 5;				// 5 мс длительность импульса
			cmd.trgDistance = 0.03f;				// триггируем через каждые 3 cм
			/* Отправляем команду */
			Send_Cmd <cmdCamTrigDist_t, SCANVIZ_CMDID_CAM_TRIG_DIST>(&cmd);
		}
		if (ch == '2') {
			cmdCamTrigTimed_t cmd;	// Команда триггера по времени
			cmd.trgPeriod_ms = 500;
			cmd.trgPulse_ms = 20;
			Send_Cmd <cmdCamTrigTimed_t, SCANVIZ_CMDID_CAM_TRIG_TIMED>(&cmd);
		}
		if (ch == '3') {
			cmdGetVersion_t cmd;	// Получить версию firmware
			cmd.reserved = 0;
			Send_Cmd <cmdGetVersion_t, SCANVIZ_CMDID_GET_VERSION>(&cmd);
		}
		if (ch == '4') {
			cmdSetOdoParams_t cmd;	// Настройка одометра - 20 Гц
			cmd.countMode = 1;
			cmd.doubleResolition = 0;
			cmd.outputRate = 2;
			cmd.reverseDir = 0;
			Send_Cmd <cmdSetOdoParams_t, SCANVIZ_CMDID_SET_ODO_PARAMS>(&cmd);
		}
		if (ch == '5') {
			cmdSetOdoParams_t cmd;	// Настройка одометра - выкл
			cmd.countMode = 0;
			cmd.doubleResolition = 0;
			cmd.outputRate = 0;
			cmd.reverseDir = 0;
			Send_Cmd <cmdSetOdoParams_t, SCANVIZ_CMDID_SET_ODO_PARAMS>(&cmd);
		}
		if (ch == '6') {
			cmdCamTrigTimed_t cmd;
			cmd.trgPulse_ms = 5;
			cmd.trgPeriod_ms = 200;
			Send_Cmd <cmdCamTrigTimed_t, SCANVIZ_CMDID_CAM_TRIG_TIMED>(&cmd);
		}
		if (ch == '7') {
			cmdSetWheelParams_t cmd;
			cmd.ticksPerRev = 150;
			cmd.tickSpacing = 0.005;
			cmd.wheelCirc = 2.033;
			Send_Cmd <cmdSetWheelParams_t, SCANVIZ_CMDID_SET_WHEEL_PARAMS>(&cmd);
		}
		if (ch == '8') {
			cmdSetEventCounter_t cmd;
			cmd.eventSrcID = 1;
			cmd.newCounterValue = 0;
			Send_Cmd <cmdSetEventCounter_t, SCANVIZ_CMDID_SET_EVENT_COUNTER>(&cmd);
		}
		if (ch == '9') {
			cmdBootloader_t cmd;
			Send_Cmd <cmdBootloader_t, SCANVIZ_CMDID_START_BOOTLOADER>(&cmd);
		}
		if (ch == '0') {
			cmdReboot_t cmd;
			cmd.rebootcode = 0;
			Send_Cmd <cmdReboot_t, SCANVIZ_CMDID_REBOOT>(&cmd);
		}
	}

	/* Завершение программы */
	if (ghEventExit) {
		SetEvent(ghEventExit);
	}
	if (hThreadRx) {
		WaitForSingleObject(hThreadRx, 500);
		CloseHandle(hThreadRx);
	}
	com.Close();
	if (hThreadParse) {
		WaitForSingleObject(hThreadParse, 500);
		CloseHandle(hThreadParse);
	}
	if (hThreadReadFromFile) {
		WaitForSingleObject(hThreadReadFromFile, 500);
		CloseHandle(hThreadReadFromFile);
	}
	if (hThreadWriteFile) {
		WaitForSingleObject(hThreadWriteFile, 500);
		CloseHandle(hThreadWriteFile);
	}
	return 0;
}

/**
 * Поток чтения из COM порта или файла
 */
DWORD WINAPI threadRx(LPVOID lpParams) {

	static uint8_t tmpbuf[65536UL];	/* промежуточный буфер 64k */

	/* Ограничить размер единовременного чтения из порта, т.к. на старых драйверах бывает BSOD */
	DWORD maxReadPortion = sizeof(tmpbuf);
	if (com.rxQueueSize && ((com.rxQueueSize / 2) < maxReadPortion)) {
		maxReadPortion = com.rxQueueSize / 2; /* Читаем по половине буфера драйвера */
	}


	while (1) {
		/* Считываем сколько пришло байт */
		DWORD rdCnt;
		BOOL rdSuccess;
		rdSuccess = com.Read(tmpbuf, maxReadPortion, &rdCnt);

		/* Read error */
		if (!rdSuccess) {
			std::cout << "COM Rx error: " << com.GetLastErrorMessage() << com.szComPortName << " Closed\n";
			com.Close();
			break;
		}
		/* Read success */
		else {
			if (rdCnt) {
				/* Отправляем в буфер для разбора */
				uint32_t wr;
				wr = sv.msgStream.Write2(tmpbuf, rdCnt);
				if (wr != rdCnt) {
					std::cout << " Parse buffer overflow! Some data lost.\n";
					sv.msgStream.Reset();
				}
				/* Запись в файл */
				wr = sbRawdata.Write2(tmpbuf, rdCnt);
				if (wr != rdCnt) {
					std::cout << " File buffer overflow! Some data lost.\n";
					sbRawdata.Reset();
				}
			}
		}

		/* Выход */
		if (WaitForSingleObject(ghEventExit, 14) != WAIT_TIMEOUT) {
			break;
		}

	}
	return 0;
}

DWORD WINAPI threadParse(LPVOID lpParams) {

	scanvizHdr_t* svHdr = (scanvizHdr_t*)0;

	while (1) {
		/* Проверяем есть ли новое сообщение */
		svHdr = sv.Get_Msg();
		if (svHdr) {
			/* смотрим ID */
			switch (svHdr->msgID) {
			case SCANVIZ_MSGID_EVENT:
				msgEvent_t* evt;
				evt = (msgEvent_t*)svHdr;	/* приводим указатеь к соответствующему типу */
				if (outEnabled) {
					/* извлекаем данные */
					std::cout << " EVT:\tTimestamp: " << PrintDbl(evt->time) << " SourceID: " << evt->srcID;
					std::cout << " Counter: " << evt->counter << "\n";
				}
				break;

			case SCANVIZ_MSGID_LOG:
				/* Для текстового лога нет отдельного типа, длина переменная, 
				   приводим к указателю на строку */
				LPSTR lpLogText;		
				lpLogText = ((LPSTR)svHdr) + sizeof(scanvizHdr_t);
				if (outEnabled) {
					std::cout << " LOG:\t" << lpLogText;
				}
				break;

			case SCANVIZ_MSGID_ACK:
				msgAck_t* ack;
				ack = (msgAck_t*)svHdr;
				if (outEnabled) {
					std::cout << " ACK:\t" << ack->errorCode << "\n";
				}
				break;

			case SCANVIZ_MSGID_ODOMETER:
				msgOdo_t* odo;
				odo = (msgOdo_t*)svHdr;
				if (outEnabled) {
					std::cout << " ODO:\tTimestamp: " << PrintDbl(odo->time) << "\tPulses: " << odo->pulses << "\n";
				}
				break;

			case SCANVIZ_MSGID_TIMEDWHEELDATA:
				msgTimedWheel_t* tw;
				tw = (msgTimedWheel_t*)svHdr;
				if (outEnabled) {
					std::cout << " TWH:\tTimestamp: " << PrintDbl(tw->timestamp) << "\tCumulative ticks: " << tw->cumulativeTicks;
					std::cout << " wheelSpeed: " << tw->wheelVel << " TicksPerRev: " << tw->ticksPerRev << "\n";
				}
				break;

			default:
				if (outEnabled) {
					std::cout << " Unknown MSGID\n";
				}
				break;
			}
			/* Освобождаем буфер сообщения для разбора следующего */
			sv.Free_Msg();
			/* повтор цикла, пока не будут обработаны все сообщения */
			continue;
		}
		if (WaitForSingleObject(ghEventExit, 14) != WAIT_TIMEOUT) {
			break;
		}
	}
	return 0;
}



DWORD WINAPI threadWriteFile(LPVOID lpParams) {

	HANDLE hFileRaw = INVALID_HANDLE_VALUE;
	hFileRaw = CreateFile("rawdata.bin", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	static uint8_t rawbuf[0x40000];
	DWORD rd;
	DWORD wr;
	while (1) {
		if (sbRawdata.GetCount() >= 0x20000) {
			rd = sbRawdata.Read2(rawbuf, 0x20000);
			WriteFile(hFileRaw, rawbuf, rd, &wr, nullptr);
		}
		if (WaitForSingleObject(ghEventExit, 30) != WAIT_TIMEOUT) {
			break;
		}
	}
	rd = sbRawdata.Read2(rawbuf, 0x40000);
	WriteFile(hFileRaw, rawbuf, rd, &wr, nullptr);
	CloseHandle(hFileRaw);
	return 0;
}

/* Поток чтения из файла */
DWORD WINAPI threadReadFromFile(LPVOID lpParams) {
	LPCSTR filename = (LPCSTR)lpParams;
	HANDLE hFileIn = INVALID_HANDLE_VALUE;
	hFileIn = CreateFile(filename, GENERIC_READ, 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFileIn == INVALID_HANDLE_VALUE) {
		std::cout << " Error open file \"" << filename << "\".\n " << com.GetLastErrorMessage() << " Q - exit.\n";
		return 1;
	}


	static uint8_t tmpbuf[65536]{};
	DWORD rdCnt;
	DWORD space;
	bool rdSuccess;
	DWORD ret = 0;

	while (1) {
		/* ожидаем освобождения места в буфере разбора */
		while ((space = sv.msgStream.GetSpace()) < 4096) {
			Sleep(14);
		}
		rdCnt = 0;
		rdSuccess = ReadFile(hFileIn, tmpbuf, space, &rdCnt, nullptr);
		if (!rdSuccess) {
			std::cout << " File Read error.\n " << com.GetLastErrorMessage() << " Q - exit.\n";
			ret = 1;
			break;
		}
		/* отправляем на разбор */
		sv.msgStream.Write2(tmpbuf, rdCnt);

		/* Выход (событие выхода) */
		if (WaitForSingleObject(ghEventExit, 14) != WAIT_TIMEOUT) {
			break;
		}
		/* Выход (конец файла) */
		if (rdCnt == 0) {
			break;
		}
	}
	return ret;
}

template <typename T, uint8_t id>
bool Send_Cmd(T* pMsg) {

	sv.Build_Msg(pMsg, id, (sizeof(T) - 10));
	size_t bytesToSend = sizeof(T);

	DWORD bytesSent = 0;
	bool success = com.Write(pMsg, bytesToSend, &bytesSent);
	if (!success) {
		std::cout << " Command send error: " << com.GetLastErrorMessage();
	}
	else {
		std::cout << " Command sent OK: " << bytesSent << "bytes\n";
	}
	return success;
}

void PrintHelp() {
	std::cout << " O - Toggle message output\n Q - exit\n H - Print this help\n\n";
	std::cout << " 1 - CAM_TRIG_DISTANCE (INSPVA)\n";
	std::cout << " 2 - CAM_TRIG_TIMED (500)\n";
	std::cout << " 3 - GET_VERSION\n";
	std::cout << " 4 - SET_ODO_PARAMS - 2 Hz\n";
	std::cout << " 5 - SET_ODO_PARAMS - OFF\n";
	std::cout << " 6 - CAM_TRIG_TIMED 200 ms\n";
	std::cout << " 7 - SET_WHEEL_PARAMS\n";
	std::cout << " 8 - SET_EVENT_COUNTER\n";
	std::cout << " 9 - START_BOOTLOADER\n";
	std::cout << " 0 - REBOOT\n";
}
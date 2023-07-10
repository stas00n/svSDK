
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <string.h>

#include "scanviz.h"
#include "svMsgProc.h"
#include "comport.h"
#include "perf.h"




/* событие для завершения потоков */
HANDLE ghEventExit;

ComPort com;
SvMsgProc sv;

enum class _mode { serial, file } mode;

DWORD WINAPI threadRx(LPVOID lpParams);
DWORD WINAPI threadReadFromFile(LPVOID lpParams);
DWORD WINAPI threadParse(LPVOID lpParams);
DWORD WINAPI threadBurn(LPVOID lpParams);
DWORD WINAPI threadWriteFileTicks(LPVOID lpParams);

uint32_t outEnabled = 1; // 

DWORD bytesRxBurn = 0;

DWORD maxTicks = 0;
DWORD sumTicks = 0;
DWORD sumCnt = 0;
DWORD maxRdCnt = 0;

CStreamBuffer<65536> sbTicks;
CStreamBuffer<0x40000> sbRawdata;

HANDLE hThreadWriteFileTicks;

template<typename T, uint8_t id>
bool Send_Cmd(T* pMsg);
void PrintHelp();

HANDLE hThreadRx;



PerfTimer tim;

int main(int argc, char** argv) {


	//HANDLE hThreadRx;
	HANDLE hThreadParse;
	HANDLE hThreadBurn;
	HANDLE hThreadReadFromFile;

	if (argc != 3) {
		std::cout << " Error syntax! Usage:\n\tsvTest comportnumber baudrate\n";
		std::cout << "\tsvTest -f filename\n Anykey for exit.\n";
		while (_getch() == 0) {
			Sleep(64);
		}
		return 0;
	}


	if (memcmp(argv[1], "-f", 3) == 0) {
		mode = _mode::file;
		hThreadReadFromFile = CreateThread(nullptr, 0, threadReadFromFile, argv[2], 0, nullptr);
	}
	else {
		mode = _mode::serial;
		uint32_t baud = 0;
		uint32_t comNum = 0;
		sscanf_s(argv[1], "%d", &comNum);
		sscanf_s(argv[2], "%d", &baud);
		if (!com.Open((uint8_t)comNum, baud)) {
			std::cout << " Error open " << com.szComPortName << "\n " << com.GetLastErrorMessage() << " Anykey for exit.\n";
			while (_getch() == 0) {
				Sleep(64);
			}
			return 0;
		}
		else {
			std::cout << " " << com.szComPortName << " open OK.\n";
			PrintHelp();

			hThreadRx = CreateThread(nullptr, 0, threadRx, nullptr, 0, nullptr);
			if (hThreadRx) {
				SetThreadPriority(hThreadRx, THREAD_PRIORITY_TIME_CRITICAL);
			}
			hThreadWriteFileTicks = CreateThread(nullptr, 0, threadWriteFileTicks, nullptr, 0, nullptr);

		}
	}
	ghEventExit = CreateEvent(nullptr, TRUE, FALSE, nullptr);
	//ghMutex = CreateMutex(nullptr, FALSE, nullptr);

	hThreadParse = CreateThread(nullptr, 0, threadParse, nullptr, 0, nullptr);
	if (hThreadParse) {
		SetThreadPriority(hThreadParse, THREAD_PRIORITY_ABOVE_NORMAL);
	}



	int ch;
	while (1) {
		ch = _getch();
		Sleep(20);

		if ((ch == 'q') || (ch == 'Q')) {
			break;
		}
		if ((ch == 'o') || (ch == 'O')) {
			outEnabled ^= 1;
			if (outEnabled) {
				std::cout << " Output enabled\n";
			}
			else {
				std::cout << " Output disabled\n"; ;
			}
		}
		if ((ch == 'h') || (ch == 'H')) {
			PrintHelp();
		}
		if ((ch == 'b') || (ch == 'B')) {
			hThreadBurn = CreateThread(nullptr, 0, threadBurn, nullptr, 0, nullptr);
			if (hThreadBurn) {
				SetThreadPriority(hThreadBurn, THREAD_PRIORITY_TIME_CRITICAL);
			}
		}
		if (ch == '1') {
			/* Создаем и заполняем тело команды (все кроме hdr и chksum */
			cmdCamTrigDist_t cmd;
			cmd.distSrc = DISTANCE_SRC_INSPVA;
			cmd.maxRate = 15;
			cmd.trgPulse_ms = 5;
			cmd.trgDistance = 3.0f;
			Send_Cmd <cmdCamTrigDist_t, SCANVIZ_CMDID_CAM_TRIG_DIST>(&cmd);
		}
		if (ch == '2') {
			cmdCamTrigTimed_t cmd;
			cmd.trgPeriod_ms = 0;
			cmd.trgPulse_ms = 20;
			Send_Cmd <cmdCamTrigTimed_t, SCANVIZ_CMDID_CAM_TRIG_TIMED>(&cmd);
		}
		if (ch == '3') {
			cmdGetVersion_t cmd;
			cmd.reserved = 0;
			Send_Cmd <cmdGetVersion_t, SCANVIZ_CMDID_GET_VERSION>(&cmd);
		}
		if (ch == '4') {
			cmdSetOdoParams_t cmd;
			cmd.countMode = 1;
			cmd.doubleResolition = 0;
			cmd.outputRate = 254;
			cmd.reverseDir = 0;
			Send_Cmd <cmdSetOdoParams_t, SCANVIZ_CMDID_SET_ODO_PARAMS>(&cmd);
		}
		if (ch == '5') {
			cmdSetOdoParams_t cmd;
			cmd.countMode = 1;
			cmd.doubleResolition = 0;
			cmd.outputRate = 0;
			cmd.reverseDir = 0;
			Send_Cmd <cmdSetOdoParams_t, SCANVIZ_CMDID_SET_ODO_PARAMS>(&cmd);
		}
	}

	/* Завершение программы */
	if (ghEventExit) {
		SetEvent(ghEventExit);
	}
	if (hThreadRx) {
		WaitForSingleObject(hThreadRx, INFINITE);
		CloseHandle(hThreadRx);
	}
	if (hThreadParse) {
		WaitForSingleObject(hThreadParse, INFINITE);
		CloseHandle(hThreadParse);
	}
	com.Close();
	return 0;

}

/**
 * Поток чтения из COM порта или файла
 */
DWORD WINAPI threadRx(LPVOID lpParams) {

	uint8_t tmpbuf[65536UL];	/* промежуточный буфер 64k */
	
	static uint32_t ticks_prev;

	/* Ограничить размер единовременного чтения из порта, т.к. на старых драйверах бывает BSOD */
	DWORD maxReadPortion = sizeof(tmpbuf);
	if (com.rxQueueSize && ((com.rxQueueSize / 2) < maxReadPortion)) {
		maxReadPortion = com.rxQueueSize / 2; /* Читаем по половине буфера драйвера */
	}

	ticks_prev = GetTickCount();
	sumTicks = 0;

	while (1) {
		tim.Start();
		uint32_t ticks_now = GetTickCount();
		uint32_t ticks = ticks_now - ticks_prev;
		sumTicks += ticks;
		sumCnt += 1;
		ticks_prev = ticks_now;
		if (ticks > maxTicks) {
			maxTicks = ticks;
		}

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
			sbTicks.Write((uint8_t*)&rdCnt, 4);
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
					sv.msgStream.Reset();
				}
				
				// Для теста максимальной загрузки буфера порта
				int64_t micros = tim.Stop();
				bytesRxBurn += rdCnt;
				if (maxRdCnt < rdCnt) {
					maxRdCnt = rdCnt;
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

		svHdr = sv.Get_Msg();

		if (svHdr) {
			switch (svHdr->msgID) {
			case SCANVIZ_MSGID_EVENT:
				msgEvent_t* evt;
				evt = (msgEvent_t*)svHdr;
				if (outEnabled) {
					std::cout << " EVT:\tTimestamp: " << (evt->time) << " SourceID: " << evt->srcID << "\n";
				}
				break;

			case SCANVIZ_MSGID_LOG:
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
					std::cout << " ODO:\tTimestamp: " << odo->time << "\tPulses: " << odo->pulses << "\n";
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



DWORD WINAPI threadBurn(LPVOID lpParams) {
	maxTicks = 0;
	std::cout << "\n CPU Burn for 10 seconds started!\n";
	bytesRxBurn = 0;
	maxRdCnt = 0;
	DWORD ticks = GetTickCount();

	while (1) {
		DWORD ms = GetTickCount() - ticks;
		if (ms > 10000) {
			break;
		}
	}
	float avgTicks = (float)sumTicks / (float)sumCnt;
	std::cout << " Burn Stop. " << bytesRxBurn << " Recived\n" << "MaxTicksRx = " << maxTicks << " AvgTicks = " << avgTicks << "\n";
	std::cout << "Max RdCnt = " << maxRdCnt;
	return 0;
}

DWORD WINAPI threadWriteFileTicks(LPVOID lpParams) {
	HANDLE hFileTicks;
	hFileTicks = CreateFile("ticks.bin", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hFileTicks == INVALID_HANDLE_VALUE) {
		std::cout << " Error open file " << "for writing\n" << com.GetLastErrorMessage();
		return 0;
	}

	HANDLE hFileRaw = INVALID_HANDLE_VALUE;
	hFileRaw = CreateFile("rawserial.bin", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

	uint8_t tmpbuf[4096];
	uint8_t rawbuf[0x40000];
	DWORD rd;
	DWORD wr;
	while (1) {
		if (sbTicks.GetCount() >= 4096) {
			rd = sbTicks.Read2(tmpbuf, 4096);
			WriteFile(hFileTicks, tmpbuf, rd, &wr, nullptr);
		}
		if (sbRawdata.GetCount() >= 0x20000) {
			rd = sbRawdata.Read2(rawbuf, 0x20000);
			WriteFile(hFileRaw, rawbuf, rd, &wr, nullptr);
		}
		if (WaitForSingleObject(ghEventExit, 30) != WAIT_TIMEOUT) {
			break;
		}
	}
	rd = sbTicks.Read2(tmpbuf, 4096);
	WriteFile(hFileTicks, tmpbuf, rd, &wr, nullptr);
	CloseHandle(hFileTicks);

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


	uint8_t tmpbuf[65536]{};
	DWORD rdCnt;
	DWORD space;
	bool rdSuccess;
	DWORD ret = 0;

	while (1) {
		/* ожидаем освобождения места в буфере разбора */
		while ((space = sv.msgStream.GetSpace()) < 4096) {
			Sleep(15);
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
		if (WaitForSingleObject(ghEventExit, 30) != WAIT_TIMEOUT) {
			break;
		}
		/* Выход (конец файла) */
		if (rdCnt == 0) {
			break;
		}
	}
	return ret;
}

template<typename T, uint8_t id>
bool Send_Cmd(T* pMsg) {

	sv.Build_Msg(pMsg, id, (sizeof(T) - 10));
	size_t bytesToSend = sizeof(T);

	DWORD bytesSent = 0;
	bool success = com.Write(pMsg, bytesToSend, &bytesSent);
	if (!success) {
		std::cout << "Command send error: " << com.GetLastErrorMessage();
	}
	else {
		std::cout << "Command sent OK: " << bytesSent << "bytes\n";
	}
	return success;
}

void PrintHelp() {
	std::cout << " 1...5 - Send test command\n O - Toggle message output\n B - Start CPU Burn Thread\n Q - exit\n H - Print this help\n";
}
#include "comport.h"

ComPort::ComPort()
{
}

ComPort::~ComPort()
{
}

bool ComPort::Open(uint8_t comNumber, uint32_t baud) {
	/* Build Full COM Name */
	sprintf_s(szComPortName, sizeof(szComPortName), "\\\\.\\COM%d", comNumber);

	/* Try open */
	hComPort = CreateFile(szComPortName, (GENERIC_READ | GENERIC_WRITE), 0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
	if (hComPort == INVALID_HANDLE_VALUE) {
		GetLastErrorMessage();
		return false;
	}

	/* Set timeouts */
	COMMTIMEOUTS to;
	memset(&to, 0, sizeof(to));
	to.ReadIntervalTimeout = MAXDWORD;
	if (!SetCommTimeouts(hComPort, &to)) {
		GetLastErrorMessage();
		Close();
		return false;
	}

	/* Set speed */
	DCB dcb {};
	dcb.DCBlength = sizeof(DCB);
	dcb.BaudRate = baud;
	dcb.fBinary = 1;
	dcb.ByteSize = 8;
	if (!SetCommState(hComPort, &dcb)) {
		GetLastErrorMessage();
		Close();
		return false;
	}

	COMMPROP cp {};
	if (GetCommProperties(hComPort, &cp)) {
		rxQueueSize = cp.dwCurrentRxQueue;
	}
	else {
		rxQueueSize = 0;
	}
	return true;

}



LPCSTR ComPort::GetLastErrorMessage() {
	DWORD e;
	e = GetLastError();
	DWORD success = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, nullptr, e, (MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US)), szErrorMessage, sizeof(szErrorMessage), nullptr);
	if (!success) {
		strcpy_s(szErrorMessage, sizeof(szErrorMessage), "Unknown Error");
	}
	return szErrorMessage;
}

void ComPort::Close() {
	if (hComPort != INVALID_HANDLE_VALUE) {
		CloseHandle(hComPort);
		hComPort = INVALID_HANDLE_VALUE;
	}
}


bool ComPort::Read(uint8_t* buf, DWORD nBytesToRead, DWORD* nBytesRead) {
	return ReadFile(hComPort, buf, nBytesToRead, nBytesRead, nullptr);
}

bool ComPort::Write(void * buf, DWORD nBytesToWrite, DWORD * nBytesWritten)
{
	DWORD tmp;
	bool ret = WriteFile(hComPort, buf, nBytesToWrite, &tmp, nullptr);
	if (nBytesWritten != nullptr) {
		*nBytesWritten = tmp;
	}
	return ret;
}

#pragma once
#include <Windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


class ComPort{
public:
	ComPort();
	~ComPort();
	bool Open(uint8_t comNumber, uint32_t baud);
	bool Read(uint8_t* buf, DWORD nBytesToRead, DWORD* nBytesRead);
	bool Write(void* buf, DWORD nBytesToWrite, DWORD* nBytesWritten = nullptr);
	void Close();
	LPCSTR GetLastErrorMessage();

private:
	HANDLE hComPort = INVALID_HANDLE_VALUE;

public:
	char   szComPortName[12]{};

	char   szErrorMessage[1024]{};
	DWORD  rxQueueSize = 0;	/* Размер буфера драйвера */
};



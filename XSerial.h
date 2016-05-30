#pragma once

#include <Windows.h>
#include <stdio.h>

class XSerial
{
public:
	XSerial();
	~XSerial();

public:
	bool Open(const char* comx, DWORD pid, UINT msgNo);
	void Close();
	bool Write(const char* data, DWORD dataLen);

private:
	bool Read(char* dst, DWORD dataLen, LPDWORD readed);
	static DWORD WINAPI evtRoutine(LPVOID param);

private:
	HANDLE       hConn, hThreadEvent;
	DWORD        pidEventRoutine, outputPid;
	OVERLAPPED   RxOs, TxOs;
	UINT         dataMsgNo; // 数据消息 输出标识
};

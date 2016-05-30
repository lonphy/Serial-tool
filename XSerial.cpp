#include "XSerial.h"

XSerial::XSerial(){
	hConn = INVALID_HANDLE_VALUE;
}

XSerial::~XSerial(){
	if (hConn != INVALID_HANDLE_VALUE)
	{
		CloseHandle(hConn);
		hConn = INVALID_HANDLE_VALUE;
	}
}

DWORD WINAPI XSerial::evtRoutine(LPVOID param)
{
	XSerial* t = (XSerial*)param;
	OVERLAPPED ol;
	memset(&ol, 0, sizeof(OVERLAPPED));
	ol.hEvent = CreateEvent(0,TRUE,FALSE,0);
	DWORD readed = 0;
	char data[128];
	while(1)
	{
		DWORD dwEvtMask, dwResult;
		WaitCommEvent(t->hConn, &dwEvtMask, &ol);
		switch ( dwResult = WaitForSingleObject(ol.hEvent, INFINITE) )
		{
		case WAIT_OBJECT_0:
			if( (dwEvtMask&EV_RXFLAG) == EV_RXFLAG)
			{
				memset(data, 0, 128);
				if (t->Read(data, 127, &readed))
				{
					char *msgData = new char[ readed +1 ];
					ZeroMemory(msgData, readed +1);
					CopyMemory(msgData, data, readed);
					// printf("EV_TXFLAG (%d):%s\r\n", readed, data);
					PostThreadMessage(t->outputPid, t->dataMsgNo, (WPARAM)msgData, (LPARAM)readed);
				}
			}
			break;
		default:
			printf("%d\r\n", dwResult);
		}
	}
	return 0;
}

bool XSerial::Open(const char* comx, DWORD pid, UINT msgNo)
{
	// step 1. open com port with async
	hConn = CreateFile(comx, GENERIC_READ|GENERIC_WRITE, 0, 0, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
	if (hConn == INVALID_HANDLE_VALUE)
	{
		printf("�򿪴���ʧ�ܣ�%d\r\n", GetLastError());
		return false;
	}

	// ���ö�д�����С
	if ( !SetupComm(hConn, 1024, 1024) )
	{
		printf("���ö�д����ʧ�ܣ�%d\r\n", GetLastError());
		return false;
	}

	DCB dcb = {};
	GetCommState(hConn, &dcb);
	dcb.BaudRate = CBR_115200; // ������
	dcb.ByteSize = 8;          // ����λ8
	dcb.StopBits = ONESTOPBIT; // ֹͣλ1
	dcb.Parity   = NOPARITY;   // У��λ��
	dcb.fParity  = 0;          // ��ֹ��żУ��
	dcb.EvtChar = '\n';        // ��ȡ���س����з���һ�����
	SetCommState(hConn, &dcb);

	COMMTIMEOUTS ct;
	ct.ReadIntervalTimeout= 1000; 
	ct.ReadTotalTimeoutConstant= 500; 
	ct.ReadTotalTimeoutMultiplier= 5000; 
	ct.WriteTotalTimeoutConstant= 500; 
	ct.WriteTotalTimeoutMultiplier=2000; 
	SetCommTimeouts(hConn, &ct);

	// ���������¼�, ��Ϊ�������¼��ַ��� ���ԣ� EV_RXCHAR���ټ���
	SetCommMask(hConn, EV_RXFLAG);

	// ȡ���������еĲ����� ����ջ���
	PurgeComm(hConn, PURGE_TXABORT|PURGE_RXABORT|PURGE_TXCLEAR|PURGE_RXCLEAR);

	outputPid = pid;
	dataMsgNo = msgNo;
	
	hThreadEvent = CreateThread(0,0, &evtRoutine, this, 0, 0);
	CloseHandle(hThreadEvent);
	return true;
}

void XSerial::Close()
{
	if (hConn == INVALID_HANDLE_VALUE)
		return;
	if (CloseHandle(hConn))
		hConn = INVALID_HANDLE_VALUE;
	if (CloseHandle(hThreadEvent))
		hThreadEvent = INVALID_HANDLE_VALUE;
}

bool XSerial::Write(const char* data, DWORD dataLen)
{
	COMSTAT    ComStat;  
	DWORD      dwErrorFlags;  
	ClearCommError(hConn, &dwErrorFlags, &ComStat);
	memset(&RxOs, 0, sizeof(OVERLAPPED));

    if ( !WriteFile(hConn, data, dataLen, &dataLen, &RxOs) ) 
    {  
        if( (dwErrorFlags=GetLastError()) ==ERROR_IO_PENDING )  
        {  
            GetOverlappedResult(hConn, &RxOs, &dataLen, TRUE);
			// printf("д������%d�ֽ�\r\n", dataLen);
            return true;  
		}
		printf("д��ʱ������, no->%d\r\n", dwErrorFlags);
		
        return false;  
    }  
	return true;
}

bool XSerial::Read(char* dst, DWORD dataLen, LPDWORD readed)
{
	*readed = 0;

	COMSTAT    ComStat;  
	DWORD      dwErrorFlags;

	memset(&TxOs, 0, sizeof(OVERLAPPED));  
	TxOs.hEvent = CreateEvent(0,TRUE,FALSE,0);
	
	// �������, ����ȡ����״̬
	ClearCommError(hConn, &dwErrorFlags, &ComStat);
	if ( ComStat.cbInQue == 0 ) { // ��������û������
		return false;
	}

	dataLen = min(dataLen, (DWORD)ComStat.cbInQue);  
 
	// �������false, ��Ҫ��һ���ж�
	if ( !ReadFile(hConn, dst, dataLen, readed, &TxOs) )
	{  
		if( (dwErrorFlags = GetLastError()) ==ERROR_IO_PENDING)
		{
			GetOverlappedResult(hConn, &TxOs, readed, TRUE);
			return true;
		}
		printf("��ȡʱ������, no->%d\r\n", dwErrorFlags);
		return false;
	}
	return true;
}
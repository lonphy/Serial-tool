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
		printf("打开串口失败：%d\r\n", GetLastError());
		return false;
	}

	// 设置读写缓冲大小
	if ( !SetupComm(hConn, 1024, 1024) )
	{
		printf("设置读写缓冲失败：%d\r\n", GetLastError());
		return false;
	}

	DCB dcb = {};
	GetCommState(hConn, &dcb);
	dcb.BaudRate = CBR_115200; // 波特率
	dcb.ByteSize = 8;          // 数据位8
	dcb.StopBits = ONESTOPBIT; // 停止位1
	dcb.Parity   = NOPARITY;   // 校检位无
	dcb.fParity  = 0;          // 禁止奇偶校检
	dcb.EvtChar = '\n';        // 读取到回车换行符算一句结束
	SetCommState(hConn, &dcb);

	COMMTIMEOUTS ct;
	ct.ReadIntervalTimeout= 1000; 
	ct.ReadTotalTimeoutConstant= 500; 
	ct.ReadTotalTimeoutMultiplier= 5000; 
	ct.WriteTotalTimeoutConstant= 500; 
	ct.WriteTotalTimeoutMultiplier=2000; 
	SetCommTimeouts(hConn, &ct);

	// 监听接受事件, 因为设置了事件字符， 所以， EV_RXCHAR不再监听
	SetCommMask(hConn, EV_RXFLAG);

	// 取消串口所有的操作， 并清空缓冲
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
			// printf("写入数据%d字节\r\n", dataLen);
            return true;  
		}
		printf("写入时出错了, no->%d\r\n", dwErrorFlags);
		
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
	
	// 清理错误, 并获取串口状态
	ClearCommError(hConn, &dwErrorFlags, &ComStat);
	if ( ComStat.cbInQue == 0 ) { // 读缓冲中没有数据
		return false;
	}

	dataLen = min(dataLen, (DWORD)ComStat.cbInQue);  
 
	// 如果返回false, 需要进一步判断
	if ( !ReadFile(hConn, dst, dataLen, readed, &TxOs) )
	{  
		if( (dwErrorFlags = GetLastError()) ==ERROR_IO_PENDING)
		{
			GetOverlappedResult(hConn, &TxOs, readed, TRUE);
			return true;
		}
		printf("读取时出错了, no->%d\r\n", dwErrorFlags);
		return false;
	}
	return true;
}
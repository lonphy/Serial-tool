#include "Worker.h"

HANDLE hStartEvent; // �߳������¼�
HANDLE hWorkerThread;
DWORD pid;

void log(HWND h, const char * msg, int done=0)
{
	SendMessage(h, WM_LOG_MSG, done, (LPARAM)msg);
}

// �����̺߳���
void SerialWrite(HWND logHandle, const char * deviceID, XSerial* xs)
{
	log(logHandle, "========== ��ʼ ==========\r\n����豸��..[ ");
	log(logHandle, deviceID);

	if ( strlen(deviceID) != 16 )
	{
		log(logHandle, " ] ...ʧ��,��Ч���豸��\r\n", 1);
		return;
	}
	log(logHandle, " ] ..OK\r\n");

	// ��������
	char cmd[] = "WID160000000000000000\r\n";
	memcpy(&cmd[5], deviceID, 16);
	

	// д������
	log(logHandle, "д������....");
	printf("<-����:23, %s", cmd);
	if (!xs->Write(cmd, 23))
		log(logHandle, "ERROR\r\n", 1);
	else
		log(logHandle,"OK\r\n");
}



DWORD WINAPI Worker(LPVOID lpParam)
{
	HWND tarHandle = (HWND)lpParam;
	printf("worker thread start~~~\r\n");
	MSG msg;
    PeekMessage(&msg, 0, WM_USER, WM_USER, PM_NOREMOVE);

    if(!SetEvent(hStartEvent)) //set thread start event 
    {
        printf("set start event failed,errno:%d\n",::GetLastError());
        return 1;
    }

	char* deviceId=0;
	char validSuccessBuf[128];
	Counter cc = {};

	XSerial* xs = new XSerial();
    while(true)
    {
        if(GetMessage(&msg,0,0,0)) //get msg from message queue
        {
            switch(msg.message)
            {
			case WM_CHANGE_COMPORT:
				
				HttpReport("", 0, &cc);
				SendMessage(tarHandle, WM_COUNT_MSG, cc.iFaild+cc.iSuccess, cc.iSuccess);

				log(tarHandle, "��ʼ���Ӵ���..");
				log(tarHandle, (char * )msg.wParam);
				if ( !xs->Open((char *)msg.wParam, pid, WM_SERIAL_MSG) )
				{
					log(tarHandle, "....ERROR\r\n");
				}
				else
				{
					log(tarHandle, "....OK\r\n");
				}
				break;

            case WM_SERIAL_WRITE:
				if (deviceId != 0)
				{
					delete[] deviceId;
					deviceId = 0;
				}
				deviceId = (char *)msg.wParam;
				memset(validSuccessBuf, 0, 128);
				sprintf(validSuccessBuf, VALID_SUCCESS_FMT, deviceId);
				SerialWrite(tarHandle, deviceId, xs);
				break;

			case WM_SERIAL_MSG:
				char *m = (char *)msg.wParam;
				printf("-> ����(%d), %s \r\n", (int)msg.lParam, m);
				log(tarHandle, m);
				if (strstr(m, validSuccessBuf) != 0)
				{
					log(tarHandle, "========== ���� ==========\r\n", 2);
				}
				delete[] m;
				// �ϱ���������
				HttpReport(deviceId, 1, &cc);
				SendMessage(tarHandle, WM_COUNT_MSG, cc.iFaild+cc.iSuccess, cc.iSuccess);
				
				break;
            }
        }
    };
    return 0;
}



DWORD InitWorker(HWND hwnd)
{

	// ���������¼�
	hStartEvent = CreateEvent(0, FALSE, FALSE, 0);
    if(hStartEvent == 0)
    {
        printf("create start event failed,errno:%d\r\n",GetLastError());
        return 0;
    }
	hWorkerThread = CreateThread(0, 0, Worker, (LPVOID)hwnd, 0, &pid);
	if(hWorkerThread == 0)
    {
        printf("start thread failed,errno:%d\n",GetLastError());
        CloseHandle(hStartEvent);
        return 0;
    }
	WaitForSingleObject(hStartEvent,INFINITE);
    CloseHandle(hStartEvent);
	return pid;
}

void DestoryWorker()
{
	CloseHandle(hStartEvent);
}


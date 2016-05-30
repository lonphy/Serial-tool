#ifndef __HTTP__
#define __HTTP__

#include <Winsock2.h>
#include <stdio.h>
#pragma comment(lib, "ws2_32.lib")

static DWORD ServerIp = 0L;

// www.geni4s.com
#define HTTP_HEADER "GET /appshare/deviceBurning/addBurningLog?key=716995d8fe9c34c332da3b537dce11f6&deviceid=%s&issuccess=%d HTTP/1.0\r\n\
Host: test.linewin.cc\r\n\
Connection: close\r\n\
Accept: application/text\r\n\
User-Agent: l5-vc-client\r\n\r\n"

typedef struct
{
	int iSuccess;
	int iFaild;
} Counter;

BOOL InitHttp();
void DestoryHttp();

int HttpReport(const char * deviceId, int ok, Counter* dst);

#endif
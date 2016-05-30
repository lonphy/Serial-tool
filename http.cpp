#include "http.h"

BOOL InitHttp() {
	WSADATA wsaData;
    int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != NO_ERROR) {
        printf("WSAStartup function failed with error: %d\n", iResult);
        return FALSE;
    }
	return TRUE;
}

void DestoryHttp() {
	WSACleanup();
}

int HttpReport(const char * deviceId, int ok, Counter * dst) {

	if ( ServerIp == 0L ) {
		HOSTENT* pHS = gethostbyname("test.linewin.cc");
		if (pHS == NULL) {
			printf("resolve hostname error\n");
			return -1;
		}
		in_addr addr;   
		CopyMemory(&addr.S_un.S_addr, pHS->h_addr_list[0], pHS->h_length);   
		ServerIp = addr.S_un.S_addr;
	}

	SOCKET conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (conn == INVALID_SOCKET) {
        printf("socket function failed with error: %ld\n", WSAGetLastError());
        return -1;
    }

    //初始化连接与端口号
    SOCKADDR_IN addrSrv;
    addrSrv.sin_addr.S_un.S_addr = ServerIp;
    addrSrv.sin_family           = AF_INET;
    addrSrv.sin_port             = htons(80);

	// 连接到服务器
	if (SOCKET_ERROR == connect(conn, (SOCKADDR*)&addrSrv, sizeof(SOCKADDR))) {
		printf("connect function failed with error: %ld\n", WSAGetLastError());
        closesocket(conn);
        return -1;
	}

	// printf("connect to server ok~~\n");

	char buf[512];
	sprintf(buf, HTTP_HEADER, deviceId, ok);
	send(conn, buf, strlen(buf), 0);
	memset(buf, 0, 512);
	recv(conn, buf, 512, 0);
	unsigned char *data = (unsigned char *)(strstr(buf, "\r\n\r\n") + 4);
	dst->iSuccess   = ( (*data  )   - 48)*1000 + (*(data+1)-48)*100 + (*(data+2)-48)*10 + *(data+3)-48;
	dst->iFaild     = ( *(data + 4) - 48)*1000 + (*(data+5)-48)*100 + (*(data+6)-48)*10 + *(data+7)-48;
	closesocket(conn);
	return 0;
}
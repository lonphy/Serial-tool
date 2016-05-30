#ifndef __WORKER_H__
#define __WORKER_H__
#include "http.h"
#include "XSerial.h"

#define WM_LOG_MSG                WM_USER + 1
#define WM_CHANGE_COMPORT         WM_USER + 2
#define WM_SERIAL_WRITE           WM_USER + 3
#define WM_SERIAL_MSG             WM_USER + 4
#define WM_COUNT_MSG              WM_USER + 5

#define VALID_SUCCESS_FMT "receive dev id is %s\r\n"

DWORD InitWorker(HWND hwnd);
void DestoryWorker();

#endif
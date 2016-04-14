#pragma once
#ifndef TR_LIB_H
#define TR_LIB_H

#include <tchar.h>
#include <Windows.h>
#include <string>
#include <stdlib.h>
#include <iostream>

#include "robotcontroller.h"
#include "serialframe.h"

using namespace std;

#ifdef DLL_EXPORT
#define DLLAPI __declspec(dllexport)
#else
#define DLLAPI __declspec(dllimport)
#endif

class DLLAPI TRComm
{
public:
	// contruction and destruction
	TRComm();
	virtual		~TRComm();

	// port initialisation											
	BOOL		InitPort(UINT portnr = 1, UINT baud = 115200,
		char parity = 'N', UINT databits = 8, UINT stopsbits = 0,
		DWORD dwCommEvents = EV_RXCHAR | EV_CTS, UINT nBufferSize = 512,
		DWORD ReadIntervalTimeout = 1000,
		DWORD ReadTotalTimeoutMultiplier = 1000,
		DWORD ReadTotalTimeoutConstant = 1000,
		DWORD WriteTotalTimeoutMultiplier = 1000,
		DWORD WriteTotalTimeoutConstant = 1000);

	// start/stop comm watching
	BOOL		StartMonitoring();
	BOOL		RestartMonitoring();
	BOOL		StopMonitoring();

	DWORD		GetWriteBufferSize();
	DWORD		GetCommEvents();
	DCB			GetDCB();
	

	//	void		WriteToPort(char* string);
	void		WriteToPort(char* string, int n);
	void		WriteToPort(LPCTSTR string);
	void		WriteToPort(BYTE* Buffer, int n);
	void		ClosePort();
	BOOL		IsOpen();

	void SendData(LPCTSTR lpszData, const int nLength);
	BOOL RecvData(LPTSTR lpszData, const unsigned int nSize);

	//modified by arthur pau, in order to get data received
	int RetriveData(BYTE rcvByte);

protected:
	//char b[100];
	// protected memberfunctions
	void		ProcessErrorMessage(char* ErrorText);
	static DWORD WINAPI CommThread(LPVOID pParam);
	static void	ReceiveChar(TRComm* port);
	static void	WriteChar(TRComm* port);

	// thread
	//CWinThread*			m_Thread;
	HANDLE			  m_Thread;

	// synchronisation objects
	CRITICAL_SECTION	m_csCommunicationSync;
	BOOL				m_bThreadAlive;

	// handles
	HANDLE				m_hShutdownEvent;  //stop发生的事件
	HANDLE				m_hComm;		   // read  
	HANDLE				m_hWriteEvent;	 // write

	// Event array. 
	// One element is used for each event. There are two event handles for each port.
	// A Write event and a receive character event which is located in the overlapped structure (m_ov.hEvent).
	// There is a general shutdown when the port is closed. 
	HANDLE				m_hEventArray[3];

	// structures
	OVERLAPPED			m_ov;
	COMMTIMEOUTS		m_CommTimeouts;
	DCB					m_dcb;

	// misc
	UINT				m_nPortNr;		//?????
	LPWSTR				m_szWriteBuffer;
	DWORD				m_dwCommEvents;
	DWORD				m_nWriteBufferSize;

	int					m_nWriteSize;

public:
	DataFrame *m_dataFrame;

	//edited by redoblue
private:
	void Stream2Frame(BYTE *stream);
};

#endif

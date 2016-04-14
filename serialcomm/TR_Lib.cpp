#include "stdafx.h"
#include "TR_Lib.h"

#include <assert.h>

TRComm::TRComm()
{

	m_hComm = NULL;
	memset(&m_dcb, 0, sizeof(DCB));

	// initialize overlapped structure members to zero
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	// create events
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_szWriteBuffer = NULL;

	m_bThreadAlive = FALSE;
	m_nWriteSize = 1;

	//edited by redoblue
	m_dataFrame = new DataFrame();
}

//
// Delete dynamic memory
//
TRComm::~TRComm()
{
	do
	{
		SetEvent(m_hShutdownEvent);
	} while (m_bThreadAlive);

	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}
	// Close Handles  
	if (m_hShutdownEvent != NULL)
		CloseHandle(m_hShutdownEvent);
	if (m_ov.hEvent != NULL)
		CloseHandle(m_ov.hEvent);
	if (m_hWriteEvent != NULL)
		CloseHandle(m_hWriteEvent);

	//TRACE("Thread ended\n");

	delete[] m_szWriteBuffer;

	//edited by redoblue
	delete m_dataFrame;
}

BOOL TRComm::InitPort(UINT  portnr,		// portnumber (1..4)
	UINT  baud,			// baudrate
	char  parity,		// parity 
	UINT  databits,		// databits 
	UINT  stopbits,		// stopbits 
	DWORD dwCommEvents,	// EV_RXCHAR, EV_CTS etc
	UINT  writebuffersize,// size to the writebuffer

	DWORD   ReadIntervalTimeout,
	DWORD   ReadTotalTimeoutMultiplier,
	DWORD   ReadTotalTimeoutConstant,
	DWORD   WriteTotalTimeoutMultiplier,
	DWORD   WriteTotalTimeoutConstant)

{
	assert(portnr > 0 && portnr < 200);

	// if the thread is alive: Kill
	if (m_bThreadAlive)
	{
		do
		{
			SetEvent(m_hShutdownEvent);
		} while (m_bThreadAlive);
		//TRACE("Thread ended\n");
	}

	// create events
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	else
		m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);
	else
		m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	else
		m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// initialize the event objects
	m_hEventArray[0] = m_hShutdownEvent;	// highest priority
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;

	// initialize critical section
	InitializeCriticalSection(&m_csCommunicationSync);

	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	m_szWriteBuffer = new WCHAR[writebuffersize];

	m_nPortNr = portnr;

	m_nWriteBufferSize = writebuffersize;
	m_dwCommEvents = dwCommEvents;

	BOOL bResult = FALSE;
	LPWSTR szPort = new WCHAR[50];
	char *szBaud = new char[50];

	// now it critical!
	EnterCriticalSection(&m_csCommunicationSync);

	// if the port is already opened: close it
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	// prepare port strings
	wsprintfW(szPort, _T("\\\\.\\COM%d"), portnr);

	// stop is index 0 = 1 1=1.5 2=2
	int mystop;
	int myparity;
	switch (stopbits)
	{
	case 0:
		mystop = ONESTOPBIT;
		break;
	case 1:
		mystop = ONE5STOPBITS;
		break;
	case 2:
		mystop = TWOSTOPBITS;
		break;
	}
	myparity = 0;
	parity = toupper(parity);
	switch (parity)
	{
	case 'N':
		myparity = 0;
		break;
	case 'O':
		myparity = 1;
		break;
	case 'E':
		myparity = 2;
		break;
	case 'M':
		myparity = 3;
		break;
	case 'S':
		myparity = 4;
		break;
	}
	sprintf_s(szBaud, 50, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, mystop);

	// get a handle to the port
	m_hComm = CreateFile(szPort,						// communication port string (COMX)
		GENERIC_READ | GENERIC_WRITE,	// read/write types
		0,								// comm devices must be opened with exclusive access
		NULL,							// no security attributes
		OPEN_EXISTING,					// comm devices must use OPEN_EXISTING
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,			// Async I/O
		0);							// template must be 0 for comm devices

	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		// port not found
		delete[] szPort;
		delete[] szBaud;

		return FALSE;
	}

	// flush the port
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// set the timeout values
	m_CommTimeouts.ReadIntervalTimeout = ReadIntervalTimeout * 1000;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = ReadTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.ReadTotalTimeoutConstant = ReadTotalTimeoutConstant * 1000;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = WriteTotalTimeoutMultiplier * 1000;
	m_CommTimeouts.WriteTotalTimeoutConstant = WriteTotalTimeoutConstant * 1000;

	// configure
	if (SetCommTimeouts(m_hComm, &m_CommTimeouts))
	{
		if (SetCommMask(m_hComm, dwCommEvents))
		{
			if (GetCommState(m_hComm, &m_dcb))
			{
				m_dcb.EvtChar = 'q';
				m_dcb.fRtsControl = RTS_CONTROL_ENABLE;		// set RTS bit high!
				m_dcb.BaudRate = baud;  // add by mrlong
				m_dcb.Parity = myparity;
				m_dcb.ByteSize = databits;
				m_dcb.StopBits = mystop;

				//if (BuildCommDCB(szBaud, &m_dcb))
				//{
				if (SetCommState(m_hComm, &m_dcb))
					Sleep(100); // normal operation... continue
				else
					ProcessErrorMessage("SetCommState()");
				//}
				//else
				//	ProcessErrorMessage("BuildCommDCB()");
			}
			else
				ProcessErrorMessage("GetCommState()");
		}
		else
			ProcessErrorMessage("SetCommMask()");
	}
	else
		ProcessErrorMessage("SetCommTimeouts()");

	delete[] szPort;
	delete[] szBaud;

	// release critical section
	LeaveCriticalSection(&m_csCommunicationSync);

	//TRACE("Initialisation for communicationport %d completed.\nUse Startmonitor to communicate.\n", portnr);

	return TRUE;
}

//
//  The CommThread Function.
//
DWORD WINAPI TRComm::CommThread(LPVOID pParam)
{
	// Cast the void pointer passed to the thread back to
	// a pointer of TRComm class
	TRComm *port = (TRComm*)pParam;

	// Set the status variable in the dialog class to
	// TRUE to indicate the thread is running.
	port->m_bThreadAlive = TRUE;

	// Misc. variables
	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	BOOL  bResult = TRUE;

	// Clear comm buffers at startup
	if (port->m_hComm)		// check if the port is opened
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// begin forever loop.  This loop will run as long as the thread is alive.
	for (;;)
	{

		// Make a call to WaitCommEvent().  This call will return immediatly
		// because our port was created as an async port (FILE_FLAG_OVERLAPPED
		// and an m_OverlappedStructerlapped structure specified).  This call will cause the 
		// m_OverlappedStructerlapped element m_OverlappedStruct.hEvent, which is part of the m_hEventArray to 
		// be placed in a non-signeled state if there are no bytes available to be read,
		// or to a signeled state if there are bytes available.  If this event handle 
		// is set to the non-signeled state, it will be set to signeled when a 
		// character arrives at the port.

		// we do this for each port!

		bResult = WaitCommEvent(port->m_hComm, &Event, &port->m_ov);

		if (!bResult)
		{
			// If WaitCommEvent() returns FALSE, process the last error to determin
			// the reason..
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING:
			{
									 // This is a normal return value if there are no bytes
									 // to read at the port.
									 // Do nothing and continue
									 break;
			}
			case 87:
			{
					   // Under Windows NT, this value is returned for some reason.
					   // I have not investigated why, but it is also a valid reply
					   // Also do nothing and continue.
					   break;
			}
			default:
			{
					   // All other error codes indicate a serious error has
					   // occured.  Process this error.
					   port->ProcessErrorMessage("WaitCommEvent()");
					   break;
			}
			}
		}
		else
		{
			// If WaitCommEvent() returns TRUE, check to be sure there are
			// actually bytes in the buffer to read.  
			//
			// If you are reading more than one byte at a time from the buffer 
			// (which this program does not do) you will have the situation occur 
			// where the first byte to arrive will cause the WaitForMultipleObjects() 
			// function to stop waiting.  The WaitForMultipleObjects() function 
			// resets the event handle in m_OverlappedStruct.hEvent to the non-signelead state
			// as it returns.  
			//
			// If in the time between the reset of this event and the call to 
			// ReadFile() more bytes arrive, the m_OverlappedStruct.hEvent handle will be set again
			// to the signeled state. When the call to ReadFile() occurs, it will 
			// read all of the bytes from the buffer, and the program will
			// loop back around to WaitCommEvent().
			// 
			// At this point you will be in the situation where m_OverlappedStruct.hEvent is set,
			// but there are no bytes available to read.  If you proceed and call
			// ReadFile(), it will return immediatly due to the async port setup, but
			// GetOverlappedResults() will not return until the next character arrives.
			//
			// It is not desirable for the GetOverlappedResults() function to be in 
			// this state.  The thread shutdown event (event 0) and the WriteFile()
			// event (Event2) will not work if the thread is blocked by GetOverlappedResults().
			//
			// The solution to this is to check the buffer with a call to ClearCommError().
			// This call will reset the event handle, and if there are no bytes to read
			// we can loop back through WaitCommEvent() again, then proceed.
			// If there are really bytes to read, do nothing and proceed.

			bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

			if (comstat.cbInQue == 0)
				continue;
		}	// end if bResult

		// Main wait function.  This function will normally block the thread
		// until one of nine events occur that require action.
		Event = WaitForMultipleObjects(3, port->m_hEventArray, FALSE, INFINITE);

		switch (Event)
		{
		case 0:
		{
				  // Shutdown event.  This is event zero so it will be
				  // the higest priority and be serviced first.

				  port->m_bThreadAlive = FALSE;

				  // Kill this thread.  break is not needed, but makes me feel better.
				  //AfxEndThread(100);
				  ::ExitThread(100);

				  break;
		}
		case 1:	// read event
		{
					GetCommMask(port->m_hComm, &CommEvent);
					if (CommEvent & EV_RXCHAR) //接收到字符，并置于输入缓冲区中 
						ReceiveChar(port);

					if (CommEvent & EV_CTS); //CTS信号状态发生变化
					if (CommEvent & EV_RXFLAG); //接收到事件字符，并置于输入缓冲区中 
					if (CommEvent & EV_BREAK);  //输入中发生中断
					if (CommEvent & EV_ERR); //发生线路状态错误，线路状态错误包括CE_FRAME,CE_OVERRUN和CE_RXPARITY 
					if (CommEvent & EV_RING); //检测到振铃指示

					break;
		}
		case 2: // write event
		{
					// Write character event from port
					WriteChar(port);
					break;
		}

		} // end switch

	} // close forever loop

	return 0;
}

//
// start comm watching
//
BOOL TRComm::StartMonitoring()
{
	//if (!(m_Thread = AfxBeginThread(CommThread, this)))
	if (!(m_Thread = ::CreateThread(NULL, 0, CommThread, this, 0, NULL)))
		return FALSE;
	//TRACE("Thread started\n");
	return TRUE;
}

//
// Restart the comm thread
//
BOOL TRComm::RestartMonitoring()
{
	//TRACE("Thread resumed\n");
	//m_Thread->ResumeThread();
	::ResumeThread(m_Thread);
	return TRUE;
}


//
// Suspend the comm thread
//
BOOL TRComm::StopMonitoring()
{
	//TRACE("Thread suspended\n");
	//m_Thread->SuspendThread();
	::SuspendThread(m_Thread);
	return TRUE;
}


//
// If there is a error, give the right message
//

void TRComm::ProcessErrorMessage(char* ErrorText)
{
	LPWSTR Temp = new WCHAR[200];

	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
		);

	wsprintfW(Temp, _T("WARNING:  %s Failed with the following error: \n%s\nPort: %d\n"),
		(char*)ErrorText,
		lpMsgBuf,
		m_nPortNr);

	MessageBox(NULL, Temp, _T("Application Error"), MB_ICONSTOP);

	LocalFree(lpMsgBuf);
	delete[] Temp;
}

//
// Write a character.
//
void TRComm::WriteChar(TRComm* port)
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;

	DWORD BytesSent = 0;
	DWORD SendLen = port->m_nWriteSize;
	ResetEvent(port->m_hWriteEvent);


	// Gain ownership of the critical section
	EnterCriticalSection(&port->m_csCommunicationSync);

	if (bWrite)
	{
		// Initailize variables
		port->m_ov.Offset = 0;
		port->m_ov.OffsetHigh = 0;

		// Clear buffer
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

		bResult = WriteFile(port->m_hComm,							// Handle to COMM Port
			port->m_szWriteBuffer,					// Pointer to message buffer in calling finction
			SendLen,	// add by mrlong
			//strlen((char*)port->m_szWriteBuffer),	// Length of message to send
			&BytesSent,								// Where to store the number of bytes sent
			&port->m_ov);							// Overlapped structure

		// deal with any error codes
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
			{
									 // continue to GetOverlappedResults()
									 BytesSent = 0;
									 bWrite = FALSE;
									 break;
			}
			default:
			{
					   // all other error codes
					   port->ProcessErrorMessage("WriteFile()");
			}
			}
		}
		else
		{
			LeaveCriticalSection(&port->m_csCommunicationSync);
		}
	} // end if(bWrite)

	if (!bWrite)
	{
		bWrite = TRUE;

		bResult = GetOverlappedResult(port->m_hComm,	// Handle to COMM port 
			&port->m_ov,		// Overlapped structure
			&BytesSent,		// Stores number of bytes sent
			TRUE); 			// Wait flag

		LeaveCriticalSection(&port->m_csCommunicationSync);

		// deal with the error code 
		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in WriteFile()");
		}
	} // end if (!bWrite)

	// Verify that the data size send equals what we tried to send
	if (BytesSent != SendLen /*strlen((char*)port->m_szWriteBuffer)*/)  // add by 
	{
		//TRACE("WARNING: WriteFile() error.. Bytes Sent: %d; Message Length: %d\n", BytesSent, strlen((char*)port->m_szWriteBuffer));
	}
}

//
// Character received. Inform the owner
//
void TRComm::ReceiveChar(TRComm* port)
{
	BOOL  bRead = TRUE;
	BOOL  bResult = TRUE;
	DWORD dwError = 0;
	DWORD BytesRead = 0;
	BYTE RXBuff;
	static int  i = 0;


	//add by liquanhai 2011-11-06  防止死锁
	if (WaitForSingleObject(port->m_hShutdownEvent, 0) == WAIT_OBJECT_0)
		return;

	EnterCriticalSection(&port->m_csCommunicationSync);

	bResult = ReadFile(port->m_hComm,		// Handle to COMM port 
		&RXBuff,				// RX Buffer Pointer
		1,					// Read one byte
		&BytesRead,			// Stores number of bytes read
		&port->m_ov)
		;		// pointer to the m_ov structure
	// deal with the error code s
	if (!bResult)
	{
		switch (dwError = GetLastError())
		{
		case ERROR_IO_PENDING:
		{
								 // asynchronous i/o is still in progress 
								 // Proceed on to GetOverlappedResults();
								 bRead = FALSE;
								 break;
		}
		default:
		{
				   // Another error has occured.  Process this error.
				   port->ProcessErrorMessage("ReadFile()");
				   break;
		}
		}
	}
	else
	{
		// ReadFile() returned complete. It is not necessary to call GetOverlappedResults()
		bRead = TRUE;
	}

	if (!bRead)
	{
		bRead = TRUE;

		bResult = GetOverlappedResult(port->m_hComm,	// Handle to COMM port 
			&port->m_ov,		// Overlapped structure
			&BytesRead,		// Stores number of bytes read
			TRUE); 			// Wait flag

		bResult = true;
		PurgeComm(port->m_hComm, PURGE_RXABORT);
		// deal with the error code 
		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in ReadFile()");
		}
	}   //close if (!bRead)

	LeaveCriticalSection(&port->m_csCommunicationSync);
	
	port->RetriveData(RXBuff);
	/*BYTE b[10];

	if (i < 10 )
	{
	b[i] = RXBuff;

	cout << RXBuff;

		i++;
	}
	else
		i = 0;*/
		
	/*
	if (RXBuff =='0')
	{
	i = 0;
	b[i] = RXBuff;
	i++;

	}
	else if (i < 10&&b[0]=='0')
	{
	b[i] = RXBuff;
	i++;
	}
	else
	i = 0;

	*/


}

//
// Write a string to the port
//
/*
void TRComm::WriteToPort(char* string)
{
assert(m_hComm != 0);

memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
strcpy(m_szWriteBuffer, string);
m_nWriteSize=strlen(string); // add by mrlong
// set event for write
SetEvent(m_hWriteEvent);
}
*/
//
// Return the device control block
//
DCB TRComm::GetDCB()
{
	return m_dcb;
}

//
// Return the communication event masks
//
DWORD TRComm::GetCommEvents()
{
	return m_dwCommEvents;
}

//
// Return the output buffer size
//
DWORD TRComm::GetWriteBufferSize()
{
	return m_nWriteBufferSize;
}

BOOL TRComm::IsOpen()
{
	return m_hComm != NULL;
}

void TRComm::ClosePort()
{
	SetEvent(m_hShutdownEvent);

	// if the port is still opened: close it 
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	// Close Handles  
	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);

	//delete [] m_szWriteBuffer;

}


void TRComm::WriteToPort(char* string, int n)
{
	assert(m_hComm != 0);
	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	memcpy(m_szWriteBuffer, string, n);
	m_nWriteSize = n;

	// set event for write
	SetEvent(m_hWriteEvent);
}

/*
void TRComm::WriteToPort(LPCTSTR string)
{
assert(m_hComm != 0);
memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
StrCpyW(m_szWriteBuffer, string);
m_nWriteSize=strlen(string);
// set event for write
SetEvent(m_hWriteEvent);
}
*/

void TRComm::WriteToPort(BYTE* Buffer, int n)
{
	assert(m_hComm != 0);
	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	memcpy(m_szWriteBuffer, Buffer, n);
	m_nWriteSize = n;

	// set event for write
	SetEvent(m_hWriteEvent);
}


void TRComm::SendData(LPCTSTR lpszData, const int nLength)
{
	assert(m_hComm != 0);
	memset(m_szWriteBuffer, 0, nLength);
	lstrcpyW(m_szWriteBuffer, lpszData);
	m_nWriteSize = nLength;
	// set event for write
	SetEvent(m_hWriteEvent);
}

BOOL TRComm::RecvData(LPTSTR lpszData, const unsigned int nSize)
{
	//
	//接收数据
	//
	assert(m_hComm != 0);
	memset(lpszData, 0, nSize);
	DWORD mylen = 0;
	DWORD mylen2 = 0;
	while (mylen < nSize) {
		if (!ReadFile(m_hComm, lpszData, nSize, &mylen2, NULL))
			return FALSE;


		mylen += mylen2;
	}

	return TRUE;
}

int TRComm::RetriveData(BYTE rcvByte)
{
	static BYTE s_state = 0;
	static BYTE s_rcvIndex = 0;
	static BYTE s_rcvLen = 0;
	static BYTE s_xorChkm = 0;
	static BYTE s_rcvFrame[sizeof(int)];

	switch (s_state)
	{
	case 0:
		if (rcvByte == DataFrame::FRAME_HEAD_1)
			s_state = 1;
		else
			s_state = 0;
		break;
	case 1:
		if (rcvByte == DataFrame::FRAME_HEAD_2)
			s_state = 2;
		else
			s_state = 0;
		break;
	case 2:
		if (rcvByte == DataFrame::FRAME_TYPE)
		{
			s_state = 3;
		}
		else
			s_state = 0;
		break;
	case 3:
		s_rcvFrame[s_rcvIndex] = rcvByte;
		s_xorChkm ^= rcvByte;
		s_rcvIndex++;
		if (s_rcvIndex == DataFrame::FRAME_DATA_LENGTH)
		{
			s_rcvIndex = 0;
			s_state = 4;
		}
		break;
	case 4:
		if (rcvByte == s_xorChkm)
			s_state = 5;
		else
		{
			s_state = 0;
			s_xorChkm = 0;
		}
		break;
	case 5:
		s_xorChkm = 0;
		s_state = 0;

		if (rcvByte == DataFrame::FRAME_TAIL)
		{
			Stream2Frame(s_rcvFrame);
			return 1;
		}
		break;
	default:
		break;
	}

	return 0;
}

void TRComm::Stream2Frame(BYTE *stream)
{
	memcpy(&this->m_dataFrame->m_limitSwichState, stream, sizeof(int));
}


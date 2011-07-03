#include <stdio.h>
#include <windows.h>
#include <winsock2.h>

SOCKET Socket;
HANDLE thread_handle;
DWORD thread_id;
WSANETWORKEVENTS netevents;
HWND Window;
UINT_PTR TimerId;
UINT AppState;
char towrite[1024], *towrite_ptr = towrite;
char outpath[1024];

DWORD WINAPI thread(LPVOID data)
{
    FILE *f = NULL;
    DWORD Ret;
    BOOLEAN done = FALSE;
    int wrlen, rdlen;
    struct timeval tv;
    struct fd_set readable, writable, exception;

    do
	{
		FD_ZERO(&readable);
		FD_ZERO(&writable);
		FD_ZERO(&exception);

		FD_SET(Socket, &readable);
		if (*towrite_ptr)
			FD_SET(Socket,&writable);
		FD_SET(Socket, &exception);

		tv.tv_sec = 1; tv.tv_usec = 0;

		Ret = select( Socket + 1, &readable, &writable, &exception, &tv );

		if ( Ret > 0 )
		{
			if (FD_ISSET(Socket, &writable))
			{
				if (*towrite_ptr)
				{
					wrlen = send(Socket, towrite_ptr, strlen(towrite_ptr), 0);
					if (wrlen > 0)
						towrite_ptr += wrlen;
					else
					{
						done = TRUE;
					}
					fprintf( stderr, "send: %d bytes\n", wrlen );
				}
				else
				{
					fprintf( stderr, "send: finished header and waiting\n" );
				}
			}
			if (FD_ISSET(Socket, &readable))
			{
				if (!f)
					f = fopen( outpath, "wb" );
				if (!f)
				{
					done = TRUE;
				}
				rdlen = recv( Socket, outpath, sizeof(outpath), 0 );
				if (rdlen > 0)
				{
					fwrite( outpath, 1, rdlen, f );
					fprintf( stderr, "recv: %d bytes\n", rdlen ); 
				}
				else
				{
					fprintf( stderr, "closed socket\n" );
					done = TRUE;
				}
			}
			if( FD_ISSET(Socket, &exception) )
			{
				done = TRUE;
			}
		}
		else if( !Ret )
		{ 
			fprintf( stderr, "timeout\r" ); fflush( stderr );
		}
		else
		{
			fprintf( stderr, "return from select: %x\n", Ret );
		}
    } while( !done );

    //fprintf( stderr, "About to overread the socket: %d\n", 
    //	     recv( Socket, outpath, sizeof(outpath), 0 ) );
    fprintf( stderr, "Last error: %d\n", WSAGetLastError() );

    if( f ) fclose( f );
}

//74.125.224.112 80 / index.html

int main( int argc, char **argv )
{
    WSADATA wdata;
    BOOLEAN got_header = FALSE;
    MSG msg;
    INT nbio = 1;
    sockaddr_in sa = { AF_INET };

    WSAStartup( 0x0101, &wdata );

    if( argc < 5 )
	{
		fprintf( stderr, "usage: %s host port path\n", argv[0]);
		return 1;
    }

    sa.sin_addr.s_addr = inet_addr( argv[1] );
    sa.sin_port = htons( atoi( argv[2] ) );

    sprintf( towrite, "GET %s HTTP/1.0\r\n\r\n", argv[3] );
    strcpy( outpath, argv[4] );

    Socket = socket( AF_INET, SOCK_STREAM, 0 );

    if( ioctlsocket( Socket, FIONBIO, (ULONG *)&nbio ) )
	{
		fprintf( stderr, "ioctlsocket: %d\n", WSAGetLastError() );
		return 1;
    }

	thread_handle = CreateThread( NULL, 
				  0, 
				  (LPTHREAD_START_ROUTINE)thread, 
				  NULL, 
				  0, 
				  &thread_id );

    fprintf( stderr, "connecting\n" );
    connect( Socket, (struct sockaddr *)&sa, sizeof(sa) );

    WaitForSingleObject( thread_handle, INFINITE );
	
	fprintf( stderr, "closing socket\n" );
    closesocket( Socket );

    WSACleanup();

    return 0;
}
 
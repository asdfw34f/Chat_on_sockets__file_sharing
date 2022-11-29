#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <synchapi.h>
#include <time.h>
#include <string.h>
#include <mswsock.h>

#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define maxBufferSize 8192
#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"

WSADATA wsaData;
SOCKET ConnectSocket = INVALID_SOCKET;

struct addrinfo* result = NULL, * ptr, hints;
int iResult;

char recvbuf[DEFAULT_BUFLEN] = { 0 };
int recvbuflen = DEFAULT_BUFLEN;
char buffer[MAX_PATH] = { 0 };

int __cdecl main(int argc, char** argv) {

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed with error: %d\n",
            iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = getaddrinfo("localhost",
        DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed with error: %d\n",
            iResult);
        WSACleanup();
        return 1;
    }

    // Attempt to connect to an address until one succeeds
    for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
        // Create a SOCKET for connecting to server
        ConnectSocket = socket(ptr->ai_family,
            ptr->ai_socktype,
            ptr->ai_protocol);

        if (ConnectSocket == INVALID_SOCKET) {
            printf("socket failed with error: %ld\n",
                WSAGetLastError());
            WSACleanup();
            return 1;
        }

        // Connect to server.
        iResult = connect(ConnectSocket,
            ptr->ai_addr,
            (int)ptr->ai_addrlen);

        if (iResult == SOCKET_ERROR) {
            closesocket(ConnectSocket);
            ConnectSocket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    BOOL isRunning_ = TRUE;
    while (isRunning_ == TRUE) {
        printf("Enter message:\n");
        scanf_s("%s", buffer, sizeof(buffer));
         
        // Send an initial buffer
        iResult = send(ConnectSocket, buffer,
            (int)strlen(buffer), 0);
        if (strncmp(buffer, "bye", strlen("bye")) == 0)
            isRunning_ = FALSE;

        //senf file
        else if (strncmp(buffer, "file", strlen("file")) == 0) {
            char *fName = { strtok(buffer, "file ") };
                char data[1024];

                DWORD cbRead;
                HANDLE hFile = CreateFileA(fName, 
                    GENERIC_READ |  // read and write access 
                    GENERIC_WRITE,
                    0,              // no sharing 
                    NULL,           // default security attributes
                    OPEN_EXISTING,  // opens existing pipe 
                    0,              // default attributes 
                    NULL);          // no template file 
                if (hFile == NULL)
                {
                    printf("error created handle of file %d", GetLastError());
                    continue;
                }
                int size = GetFileSize(hFile, 0);
                send(ConnectSocket, (char*)&size, sizeof(size), NULL);

                int shet = size;
                while (shet > 0) {

                     ReadFile(
                         hFile,    // pipe handle
                         data,    // buffer to receive reply
                         1024 * sizeof(char),  // size of buffer                                    
                         &cbRead,  // number of bytes read                                             
                         NULL);

                     send(ConnectSocket, data, cbRead, 0);

                    shet -= cbRead;
                }
        }
        if (iResult == SOCKET_ERROR)
            isRunning_ = FALSE;

        // Get an initial buffer
        memset(recvbuf, 0, sizeof(recvbuf));
        iResult = recv(ConnectSocket,
            recvbuf, recvbuflen, 0);

        if (iResult > 0)
            printf("New message: %s\n", recvbuf);
        else if (iResult == 0)
            isRunning_ = FALSE;
        else {
            printf("recv failed with error: %d\n",
                WSAGetLastError());
            isRunning_ = FALSE;
        }
        memset(recvbuf, 0, sizeof(recvbuf));
    }

    freeaddrinfo(result);
    printf("Client disconnected!\n");

    // shutdown the connection since no more data will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n",
            WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

BOOL ServerRecvAllBytes(SOCKET connection, char* data, int totalbytes) {
    int totalbytesreceived = 0;

    // if buffer is bigger then maximum allowed
    if (totalbytes > maxBufferSize) {

        //while full buffer not received
        while (totalbytesreceived < totalbytes) {
            int bytesrecv = 0;
            int bytesleft = totalbytes - totalbytesreceived;

            // if there is still more left then maximum size keep recv full size
            if (bytesleft >= maxBufferSize) {
                while (bytesrecv < maxBufferSize) {

                    int ReturnCheck = recv(connection, data + totalbytesreceived,
                        maxBufferSize - bytesrecv, NULL);
                    if (ReturnCheck == SOCKET_ERROR)
                        return FALSE;

                    bytesrecv += ReturnCheck;
                    totalbytesreceived += ReturnCheck;
                }
            }
            else { // if left less then maximum size recv last bytes left 
                while (bytesrecv < bytesleft) {
                    int ReturnCheck = recv(connection, data + totalbytesreceived,
                        bytesleft - bytesrecv, NULL);
                    if (ReturnCheck == SOCKET_ERROR)
                        return FALSE;
                    bytesrecv += ReturnCheck;
                    totalbytesreceived += ReturnCheck;
                }
            }
        }
    }
    else { // if buffer is not bigger then maximum allowed
        while (totalbytesreceived < totalbytes) {

            int ReturnCheck = recv(connection, data + totalbytesreceived,
                totalbytes - totalbytesreceived, NULL);
            if (ReturnCheck == SOCKET_ERROR) {
                return FALSE;
            }
            totalbytesreceived += ReturnCheck;
        }
    }
    return TRUE;
}

DWORD WINAPI ThreadRecv(LPVOID LP)
{
    while (1) {
        // Get an initial buffer
        memset(recvbuf, 0, sizeof(recvbuf));
        iResult = recv(ConnectSocket,
            recvbuf, recvbuflen, 0);

        if (iResult > 0) {
            char* checkfile = "file ";
            char* fil = strstr(recvbuf, checkfile);

            if (fil != NULL) {
                char* fName = strcat(checkfile, recvbuf);
                iResult = recv(ConnectSocket,
                    recvbuf, recvbuflen, 0);
                if (iResult > 0)
                    ServerRecvAllBytes(ConnectSocket, LP, recvbuflen);
            }
            else {
                printf("New message: %s\n", recvbuf);

            }
        }
        else if (iResult == 0)
            break;
        else {
            printf("recv failed with error: %d\n",
                WSAGetLastError());
            break;
        }
        memset(recvbuf, 0, sizeof(recvbuf));
    }
}
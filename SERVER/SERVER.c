﻿#undef UNICODE
#define WIN32_LEAN_AND_MEAN
#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Need to link with Ws2_32.lib
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27016"
#define MAX 256
#define maxBufferSize 8192

WSADATA wsaData;
int iResult;

SOCKET ListenSocket = INVALID_SOCKET;
SOCKET ClientSocket[2] = { INVALID_SOCKET };

struct addrinfo* result = NULL;
struct addrinfo hints;

int iSendResult;
BOOL isRunning = 1;

char MY_message[DEFAULT_BUFLEN], NEW_message[DEFAULT_BUFLEN];
int recvbuflen = DEFAULT_BUFLEN;

HANDLE ThreadClient[2], object;
DWORD IDPthreadClient;
HANDLE hFile;

BOOL ServerRecvAllBytes(SOCKET connection, char* data, int totalbytes) {

    int totalbytesreceived = 0;

    hFile = CreateFileA(data,
        GENERIC_READ |  // read and write access 
        GENERIC_WRITE,
        0,              // no sharing 
        NULL,           // default security attributes
        OPEN_EXISTING,  // opens existing pipe 
        0,              // default attributes 
        NULL);          // no template file

    // if buffer is bigger then maximum allowed
    if (totalbytes > maxBufferSize) {

        //while full buffer not received
        while (totalbytesreceived < totalbytes) {

            int bytesrecv = 0;
            int bytesleft = totalbytes - totalbytesreceived;

            // if there is still more left then maximum 
            // size keep recv full size
            if (bytesleft >= maxBufferSize) {
                while (bytesrecv < maxBufferSize) {

                    int ReturnCheck = recv(connection,
                        data + totalbytesreceived,
                        maxBufferSize - bytesrecv, NULL);
                    if (ReturnCheck == SOCKET_ERROR)
                        return FALSE;

                    WriteFile(hFile,
                        data,
                        totalbytes,
                        (DWORD)sizeof(data),
                        0);

                    bytesrecv += ReturnCheck;
                    totalbytesreceived += ReturnCheck;
                }
            }
            else { // if left less then maximum size recv last bytes left 
                while (bytesrecv < bytesleft) {

                    int ReturnCheck = recv(connection,
                        data + totalbytesreceived,
                        bytesleft - bytesrecv, NULL);

                    if (ReturnCheck == SOCKET_ERROR)
                        return FALSE;

                    WriteFile(hFile,
                        data,
                        totalbytes,
                        (DWORD)sizeof(data),
                        0);

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

DWORD WINAPI client(LPVOID lp)
{
    SOCKET client;
    memcpy(&client, lp, sizeof(SOCKET));
    BOOL isRunning_ = TRUE;
    char* recvbuf[260] = { 0 };

    while (isRunning_ == TRUE) {
        char buffer[MAX_PATH] = { 0 };
        int iResult_ = recv(client,
            buffer, sizeof(buffer), 0);

        if (ClientSocket[1] != INVALID_SOCKET && iResult_ > 0 ) {
          // char* checkfile = "file- ";
          // char* fil = strstr(recvbuf, checkfile);
          //
          // if (fil != NULL) {
          //
          //     // Get size of file
          //     iResult = recv(client,
          //         recvbuf, (int)DEFAULT_BUFLEN, 0);
          //
          //     if (iResult > 0)
          //         if (fil != NULL) {
          //             int size = 0;
          //             char* filename = "";
          //             iResult = recv(ConnectSocket,
          //                 &size, sizeof(int), 0);
          //             if (iResult > 0) {
          //                 char buffer[260] = { 0 };
          //                 int bytesrecv = 0;
          //
          //                 while (bytesrecv < size) {
          //
          //                     int ReturnCheck = recv(ConnectSocket,
          //                         buffer,
          //                         sizeof(buffer), NULL);
          //
          //                     if (ReturnCheck == SOCKET_ERROR) {
          //                         printf("recv failed with error: %d\n",
          //                             WSAGetLastError());
          //                         break;
          //                     }
          //                     if (client == ClientSocket[0]) 
          //                         send(ClientSocket[1], buffer, dwBytesRead, 0);
          //                     else
          //                         send(ClientSocket[0], buffer, dwBytesRead, 0);
          //
          //                     memset(buffer, 0, sizeof(buffer));
          //                     bytesrecv += ReturnCheck;
          //                 }
          //             }
          //         }
          // }
          //
            printf_s("Client [%d] message: %s\n",
                client, buffer);
            if (ClientSocket[1] != INVALID_SOCKET)
                if (ClientSocket[0] == client)
                    iResult_ = send(ClientSocket[1], buffer,
                        (int)strlen(buffer), 0);
                else
                    iResult_ = send(ClientSocket[0], buffer,
                        (int)strlen(buffer), 0);
        }
        else if (iResult_ == 0) {
            printf_s("Connection closed\n");
            isRunning_ = FALSE;
        }
        else {
            printf_s("recv failed with error: %d\n",
                WSAGetLastError());
            isRunning_ = FALSE;
        }
        memset(buffer, 0, sizeof(buffer));
    }
    return 0;
}

int __cdecl main(void)
{
    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf_s("WSAStartup failed with error: %d\n", 
            iResult);
        return 1;
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo(NULL,
        DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        printf_s("getaddrinfo failed with error: %d\n",
            iResult);
        WSACleanup();
        return 1;
    }

    // Create a SOCKET for the server to listen for client connections.
    ListenSocket = socket(result->ai_family,
        result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf_s("socket failed with error: %ld\n",
            WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    iResult = bind(ListenSocket,
        result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        printf_s("bind failed with error: %d\n",
            WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);

    iResult = listen(ListenSocket, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        printf_s("listen failed with error: %d\n",
            WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    object = CreateMutex(0, FALSE, 0);
    int idx = 0;
    while (isRunning == TRUE) {
        // Accept a client socket
        ClientSocket[idx] = accept(ListenSocket,
            NULL, NULL);
        if (ClientSocket[idx] == INVALID_SOCKET) {
            printf_s("accept failed with error ClientSocket: %d\n",
                WSAGetLastError());
            closesocket(ListenSocket);
            WSACleanup();
            return 1;
        }
        ThreadClient[idx] = CreateThread(NULL, 0,
            client, &ClientSocket[idx],
            0, &IDPthreadClient);
        idx++;
    }

    iResult = shutdown(ClientSocket[0], SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf_s("shutdown failed with error: %d\n",
            WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    iResult = shutdown(ClientSocket[1], SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf_s("shutdown failed with error: %d\n",
            WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // No longer need server socket
    closesocket(ListenSocket);
    WSACleanup();
    CloseHandle(ThreadClient);
    return 0;
}
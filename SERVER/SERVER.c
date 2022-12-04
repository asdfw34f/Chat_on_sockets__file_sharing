#undef UNICODE
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

// PORT
#define DEFAULT_PORT "27016"
#define SIZE_PART_FILE 1024

WSADATA wsaData;
int iResult;

SOCKET ClientSocket[2] = { INVALID_SOCKET };

HANDLE ThreadClient[2];
DWORD IDPthreadClient;

DWORD WINAPI client(LPVOID lp)
{
    BOOL isRunning_ = TRUE;
    char* recvbuf[SIZE_PART_FILE] = { 0 };
    BOOL iResult_ = TRUE;
    SOCKET client;
    memcpy(&client, lp, sizeof(SOCKET));

    while (isRunning_ == TRUE) {

        int iResult = recv(client,
            recvbuf, sizeof(recvbuf), 0);

        if (ClientSocket[1] != INVALID_SOCKET && iResult > 0 ) {
            /*// Check on a file message
            if (strncmp(recvbuf, "file-", strlen("file-")) == 0) {
                
                if (ClientSocket[1] != INVALID_SOCKET)
                    if (ClientSocket[0] == client)
                        iResult = send(ClientSocket[1], recvbuf,
                            sizeof(recvbuf), 0);
                    else
                        iResult = send(ClientSocket[0], recvbuf,
                            sizeof(recvbuf), 0);

                // Get file size
                long long int size = 0;
                iResult = recv(client,
                    &size,
                    sizeof(long long int),
                    0);

                if (iResult > 0 && size > 0) {
                    char buff_file[SIZE_PART_FILE] = { 0 };

                    if (ClientSocket[1] != INVALID_SOCKET)
                        if (ClientSocket[0] == client)
                            iResult = send(ClientSocket[1], size,
                                sizeof(size), 0);
                        else
                            iResult = send(ClientSocket[0], size,
                                sizeof(size), 0);

                    // get the file from server
                    int ReturnCheck = 0;
                    while (size > 0) {
                        ReturnCheck = recv(client,
                            &buff_file,
                            sizeof(buff_file),
                            NULL);

                        ReturnCheck = recv(client, &buff_file,
                            sizeof(buff_file), 0);

                        if (ReturnCheck == SOCKET_ERROR) {
                            printf("recv failed with error: %d\n",
                                WSAGetLastError());
                            break;
                        }
                        size -= ReturnCheck;

                        if (ClientSocket[1] != INVALID_SOCKET)
                            if (ClientSocket[0] == client)
                                ReturnCheck = send(ClientSocket[1], buff_file,
                                    sizeof(buff_file), 0);
                            else
                                ReturnCheck = send(ClientSocket[0], buff_file,
                                    sizeof(buff_file), 0);

                        size -= ReturnCheck;

                        ReturnCheck = 0;
                        memset(buff_file, 0, ReturnCheck);
                    }
                }
            }*/
                printf_s("Client [%d] message\n",
                    client);

                    if (ClientSocket[0] == client)
                        iResult_ = send(ClientSocket[1], recvbuf,
                            sizeof(recvbuf), 0);
                    else
                        iResult_ = send(ClientSocket[0], recvbuf,
                            sizeof(recvbuf), 0);

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
        memset(recvbuf, 0, sizeof(recvbuf));
    }
        if (ClientSocket[0] == client && ClientSocket[0] != NULL)
            closesocket(ClientSocket[0]);
        else if (ClientSocket[1] == client && ClientSocket[1] != NULL)
            closesocket(ClientSocket[1]);

    return 0;
}

int __cdecl main(void)
{
    SOCKET ListenSocket = INVALID_SOCKET;
    BOOL isRunning = 1;
    struct addrinfo hints;
    struct addrinfo* result = NULL;

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
﻿#define WIN32_LEAN_AND_MEAN
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

DWORD WINAPI ThreadRecv(LPVOID LP);

void fName_Cut(char buf[MAX_PATH], char* cmd)
{
    char fName[DEFAULT_BUFLEN] = { 0 };
    int j = 0;

    for (int i = strlen(cmd); i < strlen(buf); i++, j++)
        fName[j] = buf[i];

    buf = fName;
}

int __cdecl main(int argc, char** argv)
{
    HANDLE hFile;
    HANDLE hThread = 0;
    DWORD IPthread = 0;

    int iResult;
    char buffer[MAX_PATH] = { 0 };

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

    hThread = CreateThread(0, 0, ThreadRecv,
        0, 0, &IPthread);
    if (hThread == INVALID_HANDLE_VALUE)
        goto close;

    BOOL isRunning_ = TRUE;
    while (isRunning_ == TRUE) {
        printf("Enter message:\n");
        scanf_s("%s", buffer, sizeof(buffer));

        if (strncmp(buffer, "bye", strlen("bye")) == 0) {
            // Send an initial buffer
            iResult = send(ConnectSocket, buffer,
                (int)strlen(buffer), 0);
            isRunning_ = FALSE;
        } // Send file
        else if (strncmp(buffer, "file-", strlen("file-")) == 0) {

            fName_Cut(buffer, (char*)"file- ");

            hFile = CreateFileA(buffer,
                GENERIC_READ |  // read and write access 
                GENERIC_WRITE,
                0,              // no sharing 
                NULL,           // default security attributes
                OPEN_EXISTING,  // opens existing pipe 
                0,              // default attributes 
                NULL);          // no template file
            if (hFile != INVALID_HANDLE_VALUE) {
                // file transfer warning
                send(ConnectSocket, "file-",
                    strlen("file-"), NULL);

                // file size transfer
                int size = GetFileSize(hFile, 0);
                send(ConnectSocket, (char*)&size,
                    sizeof(size), NULL);

                DWORD dwBytesRead;
                memset(buffer, 0, sizeof(buffer));

                do {
                    if (ReadFile(hFile, buffer, sizeof(buffer), &dwBytesRead, NULL))
                        send(ConnectSocket, buffer, dwBytesRead, 0);
                    size -= dwBytesRead;
                } while (size > 0);
            }
            else if (hFile == NULL) {
                printf("error created handle of file %d",
                    GetLastError());
                continue;
            }
            CloseHandle(hFile);
        }
        else {
            iResult = send(ConnectSocket, buffer,
                (int)strlen(buffer), 0);
        }

        if (iResult == SOCKET_ERROR)
            isRunning_ = FALSE;
    }

    freeaddrinfo(result);
    printf("Client disconnected!\n");

    // shutdown the connection since no more data 
    // will be sent
    iResult = shutdown(ConnectSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n",
            WSAGetLastError());
        closesocket(ConnectSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
close:
    CloseHandle(hThread);
    closesocket(ConnectSocket);
    WSACleanup();
    return 0;
}

DWORD WINAPI ThreadRecv(LPVOID LP) {

    char* recvbuf[DEFAULT_BUFLEN] = { 0 };
    int iResult = 0;

    while (1) {
        // Get an initial buffer
        memset(recvbuf, 0, sizeof(recvbuf));
        iResult = recv(ConnectSocket,
            recvbuf, sizeof(recvbuf), 0);

        if (iResult > 0) {

            char* checkfile = "file- ";
            char* fil = strstr(recvbuf, checkfile);

            if (fil != NULL) {
                int size = 0;
                char* filename = "C:\Users\Daniil\Desktop\qwerth.png";
                iResult = recv(ConnectSocket,
                    &size, sizeof(int), 0);
                if (iResult > 0) {
                    char buffer[260] = { 0 };
                    int bytesrecv = 0;
                    HANDLE hFile = CreateFileA(filename,
                        GENERIC_READ |  // read and write access 
                        GENERIC_WRITE,
                        0,              // no sharing 
                        NULL,           // default security attributes
                        OPEN_EXISTING,  // opens existing pipe 
                        0,              // default attributes 
                        NULL);          // no template file
                    while (bytesrecv < size) {

                        int ReturnCheck = recv(ConnectSocket,
                            buffer,
                            sizeof(buffer), NULL);
                        if (ReturnCheck == SOCKET_ERROR) {
                            printf("recv failed with error: %d\n",
                                WSAGetLastError());
                            break;
                        }

                        WriteFile(hFile,
                            buffer,
                            ReturnCheck,
                            NULL,
                            0);
                        memset(buffer, 0, sizeof(buffer));
                        bytesrecv += ReturnCheck;
                    }
                }
            }
            else {
                printf("New message: %s\n", recvbuf);
            }
        }
        else if (iResult == 0)
            continue;
        else {
            printf("recv failed with error: %d\n",
                WSAGetLastError());
            break;
        }
        memset(recvbuf, 0, sizeof(recvbuf));
    }
}
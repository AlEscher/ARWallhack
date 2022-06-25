#include <iostream>
#include <Stdio.h>
#include <winsock2.h>
#include <Ws2tcpip.h>
#include <Windows.h>

#pragma comment(lib, "Ws2_32.lib")

constexpr auto COMM_PORT = "20042";

// Initilizes the WinSock Library for socket construction
int initWinSock() {
    WSADATA wsaData;
    int initResult;

    initResult = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (initResult) {
        printf("WinSock could not be initilized, Error code: %d", initResult);
        return initResult;
    }

    return initResult;
}

// initilazies Socket structure by fetching address, creating and binding the socket 
int initSocket(SOCKET& ListenSocket) {
    struct addrinfo* result = NULL;
    struct addrinfo hints;
    int initResult;

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the local address and port to be used by the server
    initResult = getaddrinfo(NULL, COMM_PORT, &hints, &result);
    if (initResult != 0) {
        printf("getaddrinfo failed: %d\n", initResult);
        WSACleanup();
        return initResult;
    }

    // Create a SOCKET for connecting to server
    ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (ListenSocket == INVALID_SOCKET) {
        printf("socket failed with error: %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }

    // Setup the TCP listening socket
    initResult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
    if (initResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return initResult;
    }
    freeaddrinfo(result);

    return initResult;
}

// initilizes the connection with the Client
int initConnection(SOCKET& ListenSocket, SOCKET& ClientSocket) {

    // Listen on the ListenSocket
    if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
        printf("Listen failed with error: %ld\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }

    // Accept a client socket
    ClientSocket = accept(ListenSocket, NULL, NULL);
    if (ClientSocket == INVALID_SOCKET) {
        printf("accept failed: %d\n", WSAGetLastError());
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}

// sending the first "size"-bytes of the data array through the Socket
int sendData(SOCKET& ClientSocket, char data[], int size) {
    int iSendResult;

    iSendResult = send(ClientSocket, data, size, 0);
    if (iSendResult == SOCKET_ERROR) {
        printf("send failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }
    return 0;
}

// shutdown Socket 
int shutdownSocket(SOCKET& ClientSocket) {
    int iResult;

    // shutdown the connection since we're done
    iResult = shutdown(ClientSocket, SD_SEND);
    if (iResult == SOCKET_ERROR) {
        printf("shutdown failed with error: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

    // cleanup
    closesocket(ClientSocket);
    WSACleanup();
    return 0;
}

int main()
{
    SOCKET ListenSocket = INVALID_SOCKET;
    SOCKET ClientSocket = INVALID_SOCKET;

    // init the Winsock Lib
    std::cout << initWinSock();
    // setting up the listen Socket
    std::cout << initSocket(ListenSocket);
    // accepting the connection from the client
    std::cout << initConnection(ListenSocket, ClientSocket);

    // sending some test data
    for (int i = 0; i < 6; i++) {
        char data[] = "test\n";
        int size = 5;
        sendData(ClientSocket, data, size);
        Sleep(1000);
    }
    
    // sending shutdown signals and stuff
    shutdownSocket(ClientSocket);
    return 0;
}



//---------------------------------------------------------------------
// 
// User Mode Networking
// 
//---------------------------------------------------------------------
#ifdef _WIN32

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <Windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <mswsock.h>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <iostream>
#include <string>
#include <mutex>
#include <detours.h>

using std::endl;

//#ifdef _DEBUG
#ifdef _DExfsfBUG
#define NetTrace(x) std::cout << x
#else
#define NetTrace(x) 
#endif

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pWSASend)(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesSent,
    DWORD                              dwFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pWSARecv)(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesRecvd,
    LPDWORD                            lpFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pConnect)(
    SOCKET         s,
    const sockaddr* name,
    int            namelen);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef SOCKET (WSAAPI* pWSAAccept)(
    SOCKET          s,
    sockaddr* addr,
    LPINT           addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD_PTR       dwCallbackData
);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL(*pConnectEx)(
    SOCKET s,
    const sockaddr* name,
    int namelen,
    PVOID lpSendBuffer,
    DWORD dwSendDataLength,
    LPDWORD lpdwBytesSent,
    LPOVERLAPPED lpOverlapped
    );

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL (WSAAPI* pWSAGetOverlappedResult)(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (*pClosesocket)(SOCKET s);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef int (WSAAPI* pWSAIoctl)(
    SOCKET                             s,
    DWORD                              dwIoControlCode,
    LPVOID                             lpvInBuffer,
    DWORD                              cbInBuffer,
    LPVOID                             lpvOutBuffer,
    DWORD                              cbOutBuffer,
    LPDWORD                            lpcbBytesReturned,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL (* pAcceptEx)(
    SOCKET       sListenSocket,
    SOCKET       sAcceptSocket,
    PVOID        lpOutputBuffer,
    DWORD        dwReceiveDataLength,
    DWORD        dwLocalAddressLength,
    DWORD        dwRemoteAddressLength,
    LPDWORD      lpdwBytesReceived,
    LPOVERLAPPED lpOverlapped);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef BOOL(*pGetQueuedCompletionStatus)(
    HANDLE       CompletionPort,
    LPDWORD      lpNumberOfBytesTransferred,
    PULONG_PTR   lpCompletionKey,
    LPOVERLAPPED* lpOverlapped,
    DWORD        dwMilliseconds);

//---------------------------------------------------------------------
//---------------------------------------------------------------------
typedef HANDLE(WINAPI* pCreateIoCompletionPort)(
    _In_     HANDLE    FileHandle,
    _In_opt_ HANDLE    ExistingCompletionPort,
    _In_     ULONG_PTR CompletionKey,
    _In_     DWORD     NumberOfConcurrentThreads);

class SocketInfo;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
static pWSASend _wsaSend = WSASend;
static pWSARecv _wsaRecv = WSARecv;
static pConnect _connect = connect;
static pWSAAccept _wsaAccept = WSAAccept;
static pWSAGetOverlappedResult _wsaGetOverlappedResult = WSAGetOverlappedResult;
static pClosesocket _closesocket = closesocket;
static pWSAIoctl _wsaIoctl = WSAIoctl;
static pAcceptEx _acceptEx;
static pConnectEx _connectEx;
static pGetQueuedCompletionStatus _getQueuedCompletionStatus = GetQueuedCompletionStatus;
static pCreateIoCompletionPort _createIoCompletionPort = CreateIoCompletionPort;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class CompletionPortInfo
{
public:
    HANDLE _handle;
    ULONG_PTR _key;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SocketManager
{
public:
    SocketInfo* AddSocket(SOCKET s, bool acceptSocket, int port);
    void RemoveSocket(SOCKET s);
    int SocketWrite(SOCKET s, LPWSABUF buffers, DWORD _numBuffers, LPWSAOVERLAPPED overlapped);
    int SocketRead(SOCKET s, LPWSABUF buffers, DWORD _numBuffers, LPWSAOVERLAPPED overlapped);

    BOOL GetOverlappedByteCount(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags);
    void SetOverlappedByteCount(int, LPWSAOVERLAPPED overlapped);

    SocketInfo* GetPairedSocket(SOCKET s);
    SocketInfo* GetSocket(SOCKET s);

    void SetCompletionPortInfo(SOCKET socket, HANDLE handle, ULONG_PTR key);
    CompletionPortInfo* GetCompletionPortInfo(SOCKET s);

private:
    std::map<SOCKET, CompletionPortInfo*> _completionPortInfo;
    std::map<SOCKET, SocketInfo*> _sockets;
    std::map< LPWSAOVERLAPPED, int> _overlappedByteCounts;
    std::map< LPWSAOVERLAPPED, int> _registeredByteCounts;
};


//---------------------------------------------------------------------
//---------------------------------------------------------------------
class SocketInfo
{
public:
    SocketInfo();

    void AddWriteBuffer(LPWSABUF buffer);
    int CompleteRead();

    SOCKET _s;
    bool _acceptSocket;
    int _port;
    std::vector<WSABUF> _readBuffers;
    LPWSAOVERLAPPED _readOverlapped;

    CHAR* _currentPos;
    ULONG _remainingCount;

    std::vector<LPWSABUF> _pendingReads;
};

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo::SocketInfo() :
    _s(0),
    _readOverlapped(nullptr),
    _currentPos(nullptr),
    _remainingCount(0)
{
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketManager _socketManager;
std::mutex _lock;
bool _detourEnable = true;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketInfo::AddWriteBuffer(LPWSABUF buffer)
{
    _pendingReads.emplace_back(buffer);
    if (_readOverlapped)
    {
        CompleteRead();
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int SocketInfo::CompleteRead()
{
    NetTrace("Completing Read: " << _s << endl);

    if (_currentPos == nullptr)
    {
        _currentPos = _pendingReads.front()->buf;
        _remainingCount = _pendingReads.front()->len;
    }
    auto totalWritten = 0;
    for (auto dest: _readBuffers)
    {
        auto toWrite = min(_remainingCount, dest.len);
        totalWritten += toWrite;
        memcpy(dest.buf, _currentPos, toWrite);

        _remainingCount -= toWrite;
        _currentPos += toWrite;

        if (_remainingCount == 0)
        {
            break;
        }
    }
    if (_remainingCount == 0)
    {
        _currentPos = nullptr;
        _pendingReads.erase(_pendingReads.begin());
    }
    if (_readOverlapped != nullptr)
    {
        _socketManager.SetOverlappedByteCount(totalWritten, _readOverlapped);
        if (_readOverlapped->hEvent != NULL)
        {
            SetEvent(_readOverlapped->hEvent);
        }
        auto completionInfo = _socketManager.GetCompletionPortInfo(_s);
        if (completionInfo != nullptr)
        {
            PostQueuedCompletionStatus(completionInfo->_handle, totalWritten, completionInfo->_key, _readOverlapped);
        }

    }
    _readOverlapped = nullptr;
    _readBuffers.clear();
    NetTrace("Complete Read, total written: " << totalWritten << endl);
    return totalWritten;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketManager::SetCompletionPortInfo(SOCKET socket, HANDLE handle, ULONG_PTR key)
{
    CompletionPortInfo* info = new CompletionPortInfo();
    info->_handle = handle;
    info->_key = key;
    _completionPortInfo.emplace(socket, info);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
CompletionPortInfo* SocketManager::GetCompletionPortInfo(SOCKET s)
{
    auto found = _completionPortInfo.find(s);
    if (found != _completionPortInfo.end())
    {
        return (*found).second;
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL SocketManager::GetOverlappedByteCount(SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags)
{
    auto info = GetSocket(s);
    auto found = _overlappedByteCounts.find(lpOverlapped);
    if (found != _overlappedByteCounts.end())
    {
        *lpcbTransfer = (*found).second;
        //NetTrace("Transfer complete: " << *lpcbTransfer << endl);
        _overlappedByteCounts.erase(found);
        return TRUE;
    }
    if (_registeredByteCounts.find(lpOverlapped) == _registeredByteCounts.end())
    {
        auto result = _wsaGetOverlappedResult(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
        //NetTrace("OS get overlapped result: " << result << endl);
        return result;
    }
    //NetTrace("Setting overlapped is incomplete!!!" << endl);
    WSASetLastError(WSA_IO_INCOMPLETE);
    *lpcbTransfer = 0;
    return FALSE;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketManager::SetOverlappedByteCount(int count, LPWSAOVERLAPPED overlapped)
{
    auto found = _registeredByteCounts.find(overlapped);
    if (found != _registeredByteCounts.end())
    {
        _registeredByteCounts.erase(found);
    }
    _overlappedByteCounts.emplace(overlapped, count);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo* SocketManager::AddSocket(SOCKET s, bool acceptSocket, int port)
{
    if (port == 0)
    {
        BYTE storage[0x80];
        sockaddr_in6* address = (sockaddr_in6*)storage;
        sockaddr_in* address4 = (sockaddr_in*)storage;
        memset(storage, 0, 0x80);
        int nameLen = 0x80;
        auto result = getsockname(s, (sockaddr*)address, &nameLen);
        auto error = WSAGetLastError();
        if (error == 0)
        {
            auto kind = address->sin6_family;
            port = ntohs(address4->sin_port);
            if (kind == AF_INET6)
            {
                port = ntohs(address->sin6_port);
            }
        }
    }
    auto found = _sockets.find(s);

    if (found == _sockets.end())
    {
        auto info = new SocketInfo();
        info->_s = s;
        info->_acceptSocket = acceptSocket;
        info->_port = port;
        _sockets.emplace(s, info);
        return info;
    }
    if ((*found).second->_port == 0)
    {
        (*found).second->_port = port;
    }
    return (*found).second;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void SocketManager::RemoveSocket(SOCKET s)
{
    auto found = _sockets.find(s);
    if (found != _sockets.end())
    {
        _sockets.erase(found);
    }
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo* SocketManager::GetPairedSocket(SOCKET s)
{
    auto socket = GetSocket(s);
    if (socket == nullptr)
    {
        return nullptr;
    }
    bool found0 = false;
    for (auto it : _sockets)
    {
        if ((it.second->_acceptSocket == false) && (it.first != s) && (it.second->_port == socket->_port))
        {
            return it.second;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
SocketInfo* SocketManager::GetSocket(SOCKET s)
{
    for (auto it : _sockets)
    {
        if (it.first == s)
        {
            return it.second;
        }
    }
    return nullptr;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int SocketManager::SocketRead(SOCKET s, LPWSABUF buffers, DWORD numBuffers, LPWSAOVERLAPPED overlapped)
{
    auto info = GetSocket(s);
    if (info->_port != 50051)
    {
        DWORD w = 0;
        DWORD flags = 0;
        _wsaRecv(s, buffers, numBuffers, &w, &flags, overlapped, nullptr);
        return w;
    }

    WSAOVERLAPPED localOverlapped;
    if (overlapped == nullptr)
    {
        //overlapped = &localOverlapped;
        //localOverlapped.hEvent = CreateEvent(NULL, false, false, NULL);
    }

    if (overlapped == nullptr)
    {
        if (info->_pendingReads.size() != 0)
        {
            for (int x = 0; x < numBuffers; ++x)
            {
                info->_readBuffers.emplace_back(buffers[x]);
            }
            info->_readOverlapped = overlapped;

            return info->CompleteRead();
        }
        return 0;
    }
    for (int x = 0; x < numBuffers; ++x)
    {
        info->_readBuffers.emplace_back(buffers[x]);
    }
    info->_readOverlapped = overlapped;
    _socketManager._registeredByteCounts.emplace(overlapped, 0);

    if (info->_pendingReads.size() != 0)
    {
        info->CompleteRead();
    }
    if (overlapped == &localOverlapped)
    {
        WaitForSingleObject(localOverlapped.hEvent, INFINITE);
    }
    return 0;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int SocketManager::SocketWrite(SOCKET s, LPWSABUF buffers, DWORD numBuffers, LPWSAOVERLAPPED overlapped)
{
    auto other = GetPairedSocket(s);
    //NetTrace("Writing from socket: " << s << " to socket: " << other->_s << " with " << _socketManager._sockets.size() << " total sockets" << endl);
    int byteCount = 0;
    for (int x = 0; x < numBuffers; ++x)
    {
        byteCount += buffers[x].len;
    }

    if (byteCount == 0)
    {
        return 0;
    }
    auto buffer = new CHAR[byteCount];
    auto start = buffer;
    auto totalCount = byteCount;

    for (int x = 0; x < numBuffers; ++x)
    {
        memcpy(start, buffers[x].buf, buffers[x].len);
        start += buffers[x].len;
    }
    auto pending = new WSABUF();
    pending->buf = buffer;
    pending->len = totalCount;
    //NetTrace("Wrote: " << totalCount << " bytes" << endl);
    other->AddWriteBuffer(pending);
    return totalCount;
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredConnect(SOCKET s, const sockaddr* name, int namelen)
{
    NetTrace("Connect: " << s << endl);

    sockaddr_in6* address = (sockaddr_in6*)name;
    sockaddr_in* address4 = (sockaddr_in*)name;
    auto kind = address->sin6_family;
    auto port = ntohs(address4->sin_port);
    if (kind == AF_INET6)
    {
        port = ntohs(address->sin6_port);
    }

    {
        std::lock_guard<std::mutex> lock(_lock);
        auto info = _socketManager.AddSocket(s, false, port);
    }

    auto result = _connect(s, name, namelen);
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL DetouredConnectEx(
    SOCKET s,
    const sockaddr* name,
    int namelen,
    PVOID lpSendBuffer,
    DWORD dwSendDataLength,
    LPDWORD lpdwBytesSent,
    LPOVERLAPPED lpOverlapped
)
{
    NetTrace("ConnectEx: " << s << endl);

    sockaddr_in6* address = (sockaddr_in6*)name;
    sockaddr_in* address4 = (sockaddr_in*)name;
    auto kind = address->sin6_family;
    auto port = ntohs(address4->sin_port);
    if (kind == AF_INET6)
    {
        port = ntohs(address->sin6_port);
    }

    {
        std::lock_guard<std::mutex> lock(_lock);
        auto info = _socketManager.AddSocket(s, false, port);
    }

    return _connectEx(s, name, namelen, lpSendBuffer, dwSendDataLength, lpdwBytesSent, lpOverlapped);
}


//---------------------------------------------------------------------
//---------------------------------------------------------------------
SOCKET WSAAPI DetouredWSAAccept(
    SOCKET          s,
    sockaddr* addr,
    LPINT           addrlen,
    LPCONDITIONPROC lpfnCondition,
    DWORD_PTR       dwCallbackData
)
{
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.AddSocket(s, false, 0);
    }
    return _wsaAccept(s, addr, addrlen, lpfnCondition, dwCallbackData);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredWSASend(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesSent,
    DWORD                              dwFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    //NetTrace("DetouredWSASend: " << s << ", buffers:" << dwBufferCount << endl);

    if (_detourEnable)
    {
        std::lock_guard<std::mutex> lock(_lock);
        WSASetLastError(0);
        auto written = _socketManager.SocketWrite(s, lpBuffers, dwBufferCount, lpOverlapped);
        if (lpNumberOfBytesSent != nullptr)
        {
            *lpNumberOfBytesSent = written;
        }
        return 0;
    }
    auto result = _wsaSend(s, lpBuffers, dwBufferCount, lpNumberOfBytesSent, dwFlags, lpOverlapped, lpCompletionRoutine);
    //NetTrace("Result: " << result << ", bytes sent: " << *lpNumberOfBytesSent << endl);
    return result;
}

bool _didAccept = false;

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL DetouredAcceptEx(
    SOCKET       sListenSocket,
    SOCKET       sAcceptSocket,
    PVOID        lpOutputBuffer,
    DWORD        dwReceiveDataLength,
    DWORD        dwLocalAddressLength,
    DWORD        dwRemoteAddressLength,
    LPDWORD      lpdwBytesReceived,
    LPOVERLAPPED lpOverlapped
)
{
    //NetTrace("AcceptEx: " << sListenSocket << " : " << sAcceptSocket << endl);
    {
        std::lock_guard<std::mutex> lock(_lock);
        if (!_didAccept)
        {
            _didAccept = true;
            auto info = _socketManager.AddSocket(sListenSocket, true, 0);
            info = _socketManager.AddSocket(sAcceptSocket, false, info->_port);
        }
    }
    auto result = _acceptEx(sListenSocket, sAcceptSocket, lpOutputBuffer, dwReceiveDataLength, dwLocalAddressLength, dwRemoteAddressLength, lpdwBytesReceived, lpOverlapped);
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredWSARecv(
    SOCKET                             s,
    LPWSABUF                           lpBuffers,
    DWORD                              dwBufferCount,
    LPDWORD                            lpNumberOfBytesRecvd,
    LPDWORD                            lpFlags,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    //NetTrace("DetouredWSARecv: " << s << " ov: " << lpOverlapped << endl);
    if (_detourEnable)
    {
        std::lock_guard<std::mutex> lock(_lock);
        WSASetLastError(0);
        int count = _socketManager.SocketRead(s, lpBuffers, dwBufferCount, lpOverlapped);
        if (lpOverlapped == nullptr)
        {
            *lpNumberOfBytesRecvd = count;
        }
        if (count == 0)
        {
            if (lpNumberOfBytesRecvd != nullptr)
            {
                *lpNumberOfBytesRecvd = 0;
            }
            if (lpOverlapped != nullptr)
            {
                WSASetLastError(ERROR_IO_PENDING);
            }
            else
            {
                WSASetLastError(WSAEWOULDBLOCK);
            }
            //NetTrace("wsaRecv would block: -1, error: " << WSAGetLastError() << endl);
            return -1;
        }
        //NetTrace("WSARecv did not block" << endl);
        return 0;
    }
    auto result = _wsaRecv(s, lpBuffers, dwBufferCount, lpNumberOfBytesRecvd, lpFlags, lpOverlapped, lpCompletionRoutine);
    auto error = WSAGetLastError();
    //NetTrace("wsaRecv result: " << result << ", error: " << error << endl);
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL WSAAPI DetouredWSAGetOverlappedResult(
    SOCKET          s,
    LPWSAOVERLAPPED lpOverlapped,
    LPDWORD         lpcbTransfer,
    BOOL            fWait,
    LPDWORD         lpdwFlags
)
{
    //NetTrace("DetouredWSAGetOverlappedResult: " << s << ", ov: " << lpOverlapped << endl);
    if (_detourEnable)
    {
        std::lock_guard<std::mutex> lock(_lock);
        WSASetLastError(0);
        return _socketManager.GetOverlappedByteCount(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
    }

    auto result = _wsaGetOverlappedResult(s, lpOverlapped, lpcbTransfer, fWait, lpdwFlags);
    //NetTrace("Get overlapped result: " << result << ", error: " << WSAGetLastError() << " count: " << *lpcbTransfer << endl);
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int DetouredClosesocket(SOCKET s)
{
    //NetTrace("Close Socket: " << s << endl);
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.RemoveSocket(s);
    }
    return _closesocket(s);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
int WSAAPI DetouredWSAIoctl(
    SOCKET                             s,
    DWORD                              dwIoControlCode,
    LPVOID                             lpvInBuffer,
    DWORD                              cbInBuffer,
    LPVOID                             lpvOutBuffer,
    DWORD                              cbOutBuffer,
    LPDWORD                            lpcbBytesReturned,
    LPWSAOVERLAPPED                    lpOverlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
)
{
    auto result = _wsaIoctl(s, dwIoControlCode, lpvInBuffer, cbInBuffer, lpvOutBuffer, cbOutBuffer, lpcbBytesReturned, lpOverlapped, lpCompletionRoutine);
    if (dwIoControlCode == SIO_GET_EXTENSION_FUNCTION_POINTER)
    {
        auto acceptId = GUID(WSAID_ACCEPTEX);
        auto connectId = GUID(WSAID_CONNECTEX);
        auto in = *(GUID*)lpvInBuffer;
        if (in == acceptId)
        {
            _acceptEx = *(pAcceptEx*)lpvOutBuffer;
            *((pAcceptEx*)lpvOutBuffer) = DetouredAcceptEx;
            NetTrace("Got acceptex" << endl);
        }
        if (in == connectId)
        {
            _connectEx = *(pConnectEx*)lpvOutBuffer;
            *((pConnectEx*)lpvOutBuffer) = DetouredConnectEx;
            NetTrace("Got connect" << endl);
        }
    }
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
BOOL DetouredGetQueuedCompletionStatus(
    HANDLE       CompletionPort,
    LPDWORD      lpNumberOfBytesTransferred,
    PULONG_PTR   lpCompletionKey,
    LPOVERLAPPED* lpOverlapped,
    DWORD        dwMilliseconds
)
{
    return _getQueuedCompletionStatus(CompletionPort, lpNumberOfBytesTransferred, lpCompletionKey, lpOverlapped, dwMilliseconds);
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
HANDLE WINAPI DetouredCreateIoCompletionPort(
    HANDLE    FileHandle,
    HANDLE    ExistingCompletionPort,
    ULONG_PTR CompletionKey,
    DWORD     NumberOfConcurrentThreads
)
{
    auto result = _createIoCompletionPort(FileHandle, ExistingCompletionPort, CompletionKey, NumberOfConcurrentThreads);
    if (FileHandle != NULL && FileHandle != (HANDLE)-1)
    {
        std::lock_guard<std::mutex> lock(_lock);
        _socketManager.SetCompletionPortInfo((SOCKET)FileHandle, result, CompletionKey);
    }
    return result;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
void InitDetours()
{
    DetourTransactionBegin();
    DetourUpdateThread(GetCurrentThread());
    DetourAttach(&(PVOID&)_connect, DetouredConnect);
    DetourAttach(&(PVOID&)_wsaIoctl, DetouredWSAIoctl);
    DetourAttach(&(PVOID&)_closesocket, DetouredClosesocket);
    DetourAttach(&(PVOID&)_wsaAccept, DetouredWSAAccept);
    DetourAttach(&(PVOID&)_wsaSend, DetouredWSASend);
    DetourAttach(&(PVOID&)_wsaRecv, DetouredWSARecv);
    DetourAttach(&(PVOID&)_wsaGetOverlappedResult, DetouredWSAGetOverlappedResult);
    DetourAttach(&(PVOID&)_getQueuedCompletionStatus, DetouredGetQueuedCompletionStatus);
    DetourAttach(&(PVOID&)_createIoCompletionPort, DetouredCreateIoCompletionPort);
    DetourTransactionCommit();
}

#endif

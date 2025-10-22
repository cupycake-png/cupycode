#include <iostream>
#include <sstream>
#include <cstring>

#ifdef _WIN32
    #include <windows.h>
#endif

#include "LanguageServerProcess.h"

LanguageServerProcess::LanguageServerProcess() = default;
LanguageServerProcess::~LanguageServerProcess() {stop();}

bool LanguageServerProcess::start(std::string path){
#ifdef _WIN32
    SECURITY_ATTRIBUTES securityAttributes;
    securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
    securityAttributes.bInheritHandle = 1;
    securityAttributes.lpSecurityDescriptor = NULL;

    HANDLE childStdOutRead = NULL;
    HANDLE childStdOutWrite = NULL;
    HANDLE childStdInRead = NULL;
    HANDLE childStdInWrite = NULL;

    if(!CreatePipe(&childStdOutRead, &childStdOutWrite, &securityAttributes, 0)){
        return false;
    }

    SetHandleInformation(childStdOutRead, HANDLE_FLAG_INHERIT, 0);

    if(!CreatePipe(&childStdInRead, &childStdInWrite, &securityAttributes, 0)){
        return false;
    }

    SetHandleInformation(childStdInWrite, HANDLE_FLAG_INHERIT, 0);

    STARTUPINFOA startupInfo;
    PROCESS_INFORMATION pi{};

    ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.hStdError = childStdOutWrite;
    startupInfo.hStdOutput = childStdOutWrite;
    startupInfo.hStdInput = childStdInRead;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(pi));

    BOOL success = CreateProcessA(NULL, path.data(), NULL, NULL, 1, CREATE_NO_WINDOW, NULL, NULL, &startupInfo, &pi);

    CloseHandle(childStdInRead);
    CloseHandle(childStdOutWrite);

    if(!success){
        printf("[ERR] Error creating LS process. Code: %d\n", GetLastError());
        return false;
    }

    processInfo = new PROCESS_INFORMATION(pi);

    handleWriteToServer = childStdInWrite;
    handleReadFromServer = childStdOutRead;
#else
    if(pipe(toServer) < 0 || pipe(fromServer) < 0){
        return false;
    }

    processID = fork();

    if(processID == 0){
        dup2(toServer[0], STDIN_FILENO);
        dup2(fromServer[1], STDOUT_FILENO);
        
        close(toServer[1]);
        close(fromServer[0]);
        
        execl(path.c_str(), path.c_str(), NULL);
        
        _exit(1);
    }

    close(toServer[0]);
    close(fromServer[1]);
#endif

    running = true;
    readerThread = std::thread(&LanguageServerProcess::readLoop, this);

    return true;
}

void LanguageServerProcess::stop(){
    if(!running){
        return;
    }

    running = false;

#ifdef _WIN32
    if(processInfo){
        PROCESS_INFORMATION* pi = reinterpret_cast<PROCESS_INFORMATION*>(processInfo);

        TerminateProcess(pi->hProcess, 0);
        CloseHandle(pi->hProcess);
        CloseHandle(pi->hThread);
        delete pi;

        processInfo = nullptr;
    }

    if(handleWriteToServer){
        CloseHandle(reinterpret_cast<HANDLE>(handleWriteToServer));
        handleWriteToServer = nullptr;
    }

    if(handleReadFromServer){
        CloseHandle(reinterpret_cast<HANDLE>(handleReadFromServer));
        handleReadFromServer = nullptr;
    }
#else
    if(processID > 0){
        kill(processID, SIGTERM);
        waitpid(processID, NULL, 0);
        
        processID = -1;
    }

    if(toServer[1]){
        close(toServer[1]);
    }

    if(fromServer[0]){
        close(fromServer[0]);
    }
#endif

    if(readerThread.joinable()){
        readerThread.join();
    }

    std::queue<std::string> empty;
    std::swap(messageQueue, empty);
}

// TODO: Change to add error handling
void LanguageServerProcess::send(std::string jsonMsg){
    std::ostringstream header;

    header << "Content-Length: " << jsonMsg.size() << "\r\n\r\n";
    
    std::string full = header.str() + jsonMsg;

#ifdef _WIN32
    DWORD written;
    WriteFile(reinterpret_cast<HANDLE>(handleWriteToServer), full.c_str(), (DWORD)full.size(), &written, NULL);

#else
    write(toServer[1], full.c_str(), full.size());
#endif
}

bool LanguageServerProcess::readChunk(std::string &out){
    char buff[1024];

#ifdef _WIN32
    DWORD bytesRead = 0;
    if(!ReadFile(reinterpret_cast<HANDLE>(handleReadFromServer), buff, sizeof(buff), &bytesRead, NULL)){
        printf("[ERR] ReadFile in readChunk failed. Code: %d\n", GetLastError());
        return false;
    }

    out.assign(buff, bytesRead);

#else
    ssize_t n = read(fromServer[0], buff, sizeof(buff));

    if(n <= 0){
        return false;
    }

    out.assign(buff, n);
#endif

    return true;
}

void LanguageServerProcess::readLoop(){
    while(running){
        std::string chunk = "";

        if(!readChunk(chunk)){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            continue;
        }

        readBuff += chunk;

        while(true){
            size_t headerEnd = readBuff.find("\r\n\r\n");

            if(headerEnd == std::string::npos){
                break;
            }

            std::string header = readBuff.substr(0, headerEnd);

            size_t lenPos = header.find("Content-Length:");

            if(lenPos == std::string::npos){
                break;
            }

            size_t lengthValueStart = lenPos + 15;
        
            while(lengthValueStart < header.size() && isspace((unsigned char)header[lengthValueStart])){
                lengthValueStart++;
            }

            size_t lengthValueEnd = header.find("\r\n", lengthValueStart);

            int contentLength = std::stoi(header.substr(lengthValueStart, lengthValueEnd - lengthValueStart));
        
            size_t totalLength = headerEnd + 4 + contentLength;

            if(readBuff.size() < totalLength){
                break;
            }

            std::string body = readBuff.substr(headerEnd + 4, contentLength);

            std::lock_guard<std::mutex> lock(queueMutex);
            
            messageQueue.push(body);

            queueConditionVar.notify_one();
            readBuff.erase(0, totalLength);
        }
    }
}

bool LanguageServerProcess::getNextMessage(std::string &msgOut){
    std::lock_guard<std::mutex> lock(queueMutex);

    if(messageQueue.empty()){
        return false;
    }

    msgOut = std::move(messageQueue.front());

    messageQueue.pop();

    return true;
}
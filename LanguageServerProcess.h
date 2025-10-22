#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <atomic>

#ifndef _WIN32
    #include <unistd.h>
    #include <sys/types.h>
    #include <sys/wait.h>
    #include <signal.h>
    #include <fcntl.h>
#endif

class LanguageServerProcess{
    public:
        LanguageServerProcess();
        ~LanguageServerProcess();

        bool start(std::string path);
        void send(std::string jsonMsg);
        bool getNextMessage(std::string &msgOut);
        void stop();

    private:
        void readLoop();
        bool readChunk(std::string &out);

        std::thread readerThread;
        std::mutex queueMutex;
        std::condition_variable queueConditionVar;
        std::queue<std::string> messageQueue;
        std::atomic<bool> running{false};

        std::string readBuff;

#ifdef _WIN32
        // Keep as void because of weird include problems with windows.h and raylib
        void* processInfo{};
        void* handleReadFromServer = nullptr;
        void* handleWriteToServer = nullptr;
#else
        int toServer[2]{};
        int fromServer[2]{};
        pid_t processID = -1;
#endif
};
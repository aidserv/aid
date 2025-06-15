#include "Logger.h"
#include <iostream>
#include <fstream>
#include <chrono>
#include <cstdarg>
#ifdef _DEBUG
Logger logger(Logger::OutputType::Both, "aid2.log");
#else
Logger logger(Logger::OutputType::File, "aid2.log");
#endif // _DEBUG



Logger::Logger(OutputType type, std::string fileName ) :
    m_outputType(type),
    m_fileName(fileName)
{
    if (m_outputType == OutputType::File || m_outputType == OutputType::Both) {
        m_outFile.open(m_fileName, std::ios_base::app);
    }
}

Logger::~Logger() {
    if (m_outputType == OutputType::File || m_outputType == OutputType::Both) {
        m_outFile.close();
    }
}

void Logger::log(const char* format, ...){
    std::lock_guard<std::mutex> lock(m_mutex);
    std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm localTime;
    localtime_s(&localTime, &now_c);
    std::chrono::duration<double, std::milli> ms = now.time_since_epoch() - std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch());
    char timeStr[100];
    std::strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTime);
    char message[1024];
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    std::string outStr = std::string(timeStr) + "." + std::to_string(static_cast<long long>(ms.count())) + " " + message;

    if (m_outputType == OutputType::Console || m_outputType == OutputType::Both) {
        std::cout << outStr << std::endl;
    }

    if (m_outputType == OutputType::File || m_outputType == OutputType::Both) {
        m_outFile << outStr << std::endl;
    }
}
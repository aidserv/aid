#ifndef LOGGER_H
#define LOGGER_H
#include <fstream>
#include <string>
#include <mutex>

class Logger {
public:
    enum class OutputType {
        Console,
        File,
        Both
    };
    Logger(OutputType type, std::string fileName = "");
    ~Logger();
    void log(const char* format, ...);

private:
    OutputType m_outputType;
    std::string m_fileName;
    std::ofstream m_outFile;
    std::mutex m_mutex;

};

extern Logger logger;

#endif
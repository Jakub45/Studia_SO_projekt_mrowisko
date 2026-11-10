#include <iostream>
#include <stdio.h>
#include <string.h>

#include "error_handler.hpp"
#include <ctime>


//---------------------( Obsluga bledow )---------------------------//
const char* ErrorTypeToString(ErrorType error)
{
    switch (error) {
        case ErrorType::ERROR_OK: return "ERROR_OK";
        case ErrorType::ERROR_FILE_NOT_FOUND: return "ERROR_FILE_NOT_FOUND";
        case ErrorType::ERROR_PIPE: return "ERROR_PIPE";
        case ErrorType::ERROR_DUP2: return "ERROR_DUP2";
        case ErrorType::ERROR_FORK: return "ERROR_FORK";
        case ErrorType::ERROR_EXEC: return "ERROR_EXEC";
        case ErrorType::ERROR_WRITE: return "ERROR_WRITE";
        case ErrorType::ERROR_WAITPID: return "ERROR_WAITPID";
        case ErrorType::ERROR_ARGUMENT: return "ERROR_ARGUMENT";
        case ErrorType::ERROR_PARSE: return "ERROR_PARSE";
        case ErrorType::ERROR_BUFFER_OVERFLOW: return "ERROR_BUFFER_OVERFLOW";
        case ErrorType::ERROR_POLL: return "ERROR_POLL";
        case ErrorType::ERROR_CLOCK: return "ERROR_CLOCK";
        default: return "ERROR_UNKNOWN";
    }
}

const char* ErrorSourceToString(ErrorSource source)
{
    switch (source) {
        case ErrorSource::SOURCE_ANT: return "ANT";
        case ErrorSource::SOURCE_MAIN_ANTHILL: return "MAIN_ANTHILL";
        case ErrorSource::SOURCE_TUI: return "TUI";
        case ErrorSource::SOURCE_ANT_QUEEN: return "ANT_QUEEN";
        default: return "UNKNOWN";
    }
}

void Error( ErrorType error, ErrorSource source, const char* comment, int systemError) {

    time_t t = time(NULL);
    struct tm* czas = localtime(&t);

    FILE* file = fopen("../logs/error.log", "a");

    if (file != NULL) {
        fprintf(file, "[%02d:%02d:%02d] [%s] [%s] %s %s \n",
            czas->tm_hour,
            czas->tm_min,
            czas->tm_sec,
            ErrorTypeToString(error),
            ErrorSourceToString(source),
            comment != nullptr ? comment : "",
            systemError != 0 ? strerror(systemError) : ""
        );

        fclose(file);
    }
}
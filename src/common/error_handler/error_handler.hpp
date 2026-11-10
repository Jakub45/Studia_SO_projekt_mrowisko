#pragma once

enum class ErrorType {
    ERROR_OK,
    ERROR_FILE_NOT_FOUND,
    ERROR_PIPE,
    ERROR_DUP2,
    ERROR_FORK,
    ERROR_EXEC,
    ERROR_WRITE,
    ERROR_READ,
    ERROR_WAITPID,
    ERROR_ARGUMENT,
    ERROR_PARSE,
    ERROR_BUFFER_OVERFLOW,
    ERROR_POLL,
    ERROR_CLOCK,
    ERROR_UNKNOWN
};

enum class ErrorSource {
    SOURCE_OK,
    SOURCE_ANT,
    SOURCE_MAIN_ANTHILL,
    SOURCE_TUI,
    SOURCE_ANT_QUEEN,
    SOURCE_UNOKNOWN
};

void Error(ErrorType error, ErrorSource source, const char* comment, int systemError = errno);

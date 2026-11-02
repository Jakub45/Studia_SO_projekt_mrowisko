#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

enum class Queen_status {
    QUEEN_DEAD = 0,
    QUEEN_BORN = 1,
};

struct Ant_Queen {
    Queen_status queen_status = Queen_status::QUEEN_BORN;
    int queen_lifetime;
};



/*
//--------------------( Tworzenie procesu mrowki )--------------------//
pid_t Ant_born(int i, int antPipe[2]) {
    pid_t pid = fork();

    if (pid == -1) {
        std::cerr << ToString(ErrorCode::ForkError) << '\n';
        return -1;
    }

    if (pid == 0) {
        close(antPipe[0]);

        if (dup2(antPipe[1], STDOUT_FILENO) == -1) {
            std::cerr << ToString(ErrorCode::Dup2Error) << '\n';
            _exit(static_cast<int>(ErrorCode::Dup2Error));
        }

        close(antPipe[1]);

        std::string antId = std::to_string(i);
        std::string antPath = ExeDir() + "/ant";

        execl(antPath.c_str(),
              "ant",
              antId.c_str(),
              static_cast<char*>(nullptr));

        std::cerr << ToString(ErrorCode::ExecErrorAnt) << '\n';
        _exit(static_cast<int>(ErrorCode::ExecErrorAnt));
    }

    return pid;
}

*/
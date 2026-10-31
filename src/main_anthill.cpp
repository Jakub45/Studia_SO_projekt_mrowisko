#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

enum class status {
    ANT_BORN = 0,
    ANT_DEAD = 1,
    FOOD_FOUND = 2,
};

// Obsługa błędów
enum class ErrorCode {
    Ok = 0,
    FileNotFound = 1,
    PipeError = 2,
    Dup2Error = 3,
    ForkError = 4,
    ExecErrorAnt = 5
};

const char* ToString(ErrorCode e) {
    switch (e) {
    case ErrorCode::Ok:           return "Ok";
    case ErrorCode::FileNotFound: return "Plik nie znaleziony";
    case ErrorCode::PipeError:    return "Blad potoku";
    case ErrorCode::Dup2Error:    return "Blad przekierowania deskryptora";
    case ErrorCode::ForkError:    return "Blad utworzenia procesu";
    case ErrorCode::ExecErrorAnt: return "Niepowodzenie uruchomienia procesu ant";
    }

    return "Nieznany blad";
}

// Zwraca katalog, w ktorym lezy uruchomiony program
std::string ExeDir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
    if (len == -1) {
        return ".";
    }
    path[len] = '\0';

    std::string s(path);
    return s.substr(0, s.find_last_of('/'));
}

// Tworzenie procesu TUI
pid_t StartTui(int readFd, int writeFd) {
    pid_t pid = fork();

    if (pid == -1) {
        return -1;
    }

    if (pid == 0) {
        close(writeFd);   // tui nie pisze do swojego pipe'a

        std::string fd = std::to_string(readFd);
        std::string path = ExeDir() + "/tui";

        execl(path.c_str(),
              "tui",
              fd.c_str(),
              static_cast<char*>(nullptr));

        std::cerr << "Blad uruchamiania TUI\n";
        _exit(1);
    }

    return pid;
}

// Tworzenie procesu mrówki
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

// Tworzenie potoku
int CreatePipe(int (&pipeFd)[2]) {
    if (pipe(pipeFd) == -1) {
        std::cerr << ToString(ErrorCode::PipeError) << '\n';
        return static_cast<int>(ErrorCode::PipeError);
    }

    return 0;
}

int main() {
    // Parametry
    // TODO

    int antPipe[2];
    int tuiPipe[2];

    // Utworzenie potoku mrówek
    if (CreatePipe(antPipe) != 0) {
        return static_cast<int>(ErrorCode::PipeError);
    }

    // Utworzenie potoku do TUI
    if (CreatePipe(tuiPipe) != 0) {
        close(antPipe[0]);
        close(antPipe[1]);
        return static_cast<int>(ErrorCode::PipeError);
    }

    // Uruchomienie TUI z końcem odczytującym tuiPipe
    pid_t tuiPid = StartTui(tuiPipe[0], tuiPipe[1]);

    if (tuiPid == -1) {
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
        return 1;
    }

    // main nie potrzebuje własnej kopii końca odczytującego TUI
    close(tuiPipe[0]);

    std::vector<pid_t> processes;

    // Utworzenie 10 procesów mrówek
    for (int i = 1; i <= 10; ++i) {
        pid_t pid = Ant_born(i, antPipe);

        if (pid == -1) {
            continue;
        }

        processes.push_back(pid);
    }

    // main nie zapisuje bezpośrednio do antPipe
    close(antPipe[1]);

    char buffer[256];
    std::string pending;

    while (true) {
        ssize_t bytesRead = read(antPipe[0], buffer, sizeof(buffer));

        if (bytesRead > 0) {
            pending.append(buffer, static_cast<std::size_t>(bytesRead));

            std::size_t pos;

            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string message = "EVENT " + pending.substr(0, pos + 1);
                pending.erase(0, pos + 1);

                std::size_t sent = 0;

                while (sent < message.size()) {
                    ssize_t bytesWritten = write(
                        tuiPipe[1],
                        message.data() + sent,
                        message.size() - sent
                    );

                    if (bytesWritten > 0) {
                        sent += static_cast<std::size_t>(bytesWritten);
                    }
                    else if (bytesWritten == -1 && errno == EINTR) {
                        continue;
                    }
                    else {
                        std::cerr << "Blad write() do TUI\n";
                        break;
                    }
                }
            }

            continue;
        }

        if (bytesRead == 0) {
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        std::cerr << "Blad read() z antPipe: "
                  << std::strerror(errno) << '\n';
        break;
    }

    // Przekazanie ostatniego komunikatu bez znaku nowej linii
    if (!pending.empty()) {
        std::size_t sent = 0;

        while (sent < pending.size()) {
            ssize_t bytesWritten = write(
                tuiPipe[1],
                pending.data() + sent,
                pending.size() - sent
            );

            if (bytesWritten > 0) {
                sent += static_cast<std::size_t>(bytesWritten);
            }
            else if (bytesWritten == -1 && errno == EINTR) {
                continue;
            }
            else {
                std::cerr << "Blad przekazywania ostatniego komunikatu do TUI\n";
                break;
            }
        }
    }

    close(antPipe[0]);

    // Koniec danych dla TUI
    close(tuiPipe[1]);

    // Oczekiwanie na zakończenie mrówek
    for (pid_t pid : processes) {
        int childStatus;

        while (waitpid(pid, &childStatus, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }

            std::cerr << "Blad waitpid(): "
                      << std::strerror(errno) << '\n';
            break;
        }
    }

    // Oczekiwanie na zakończenie TUI
    int tuiStatus;

    while (waitpid(tuiPid, &tuiStatus, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }

        std::cerr << "Blad waitpid() dla TUI: "
                  << std::strerror(errno) << '\n';
        break;
    }

    std::cout << "Wszystkie procesy zakonczone.\n";

    return 0;
}
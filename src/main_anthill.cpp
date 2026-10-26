#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

enum class status {
    ANT_BORN = 0,
    ANT_DEAD = 1,
    FOOD_FOUND = 2,
};

int main()
{
    int pipeFd[2];

    // Utworzenie potoku do komunikacji z procesami mrowek
    if (pipe(pipeFd) == -1) {
        std::cerr << "Blad pipe(): " << std::strerror(errno) << '\n';
        return 1;
    }

    std::vector<pid_t> processes;

    // Uruchomienie 10 procesow mrowek
    for (int i = 1; i <= 10; ++i) {
        pid_t pid = fork();

        if (pid == -1) {
            std::cerr << "Blad fork(): " << std::strerror(errno) << '\n';
            continue;
        }

        if (pid == 0) {
            // Proces potomny
            close(pipeFd[0]);
            // Przekierowanie stdout mrowki do potoku
            if (dup2(pipeFd[1], STDOUT_FILENO) == -1) {
                std::cerr << "Blad dup2(): " << std::strerror(errno) << '\n';
                _exit(1);
            }
            close(pipeFd[1]);
            // Przygotowanie argumentu z numerem mrowki
            std::string antId = std::to_string(i);
            // Uruchomienie programu ant
            execl("./ant", "ant", antId.c_str(),
                static_cast<char*>(nullptr));
            // Ta linia wykona sie tylko, jesli execl() sie nie powiedzie
            std::cerr << "Nie mozna uruchomic ./ant: "
                << std::strerror(errno) << '\n';
            _exit(127);
        }

        // Proces macierzysty zapamietuje PID potomka
        processes.push_back(pid);
    }

    // Proces macierzysty nie bedzie juz pisal do potoku
    close(pipeFd[1]);

    char buffer[256];
    std::string pending;

    while (true) {
        ssize_t bytesRead = read(pipeFd[0], buffer, sizeof(buffer));

        if (bytesRead > 0) {
            pending.append(buffer, static_cast<std::size_t>(bytesRead));

            std::size_t pos;

            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string message = pending.substr(0, pos);
                pending.erase(0, pos + 1);

                if (message.find("FOOD_FOUND ") == 0) {
                    std::cout << message.substr(11)
                        << " znalazl jedzenie!" << std::endl;
                }
                else if (message.find("ANT_DEAD ") == 0) {
                    std::cout << message.substr(9)
                        << " umarl!" << std::endl;
                }
                else {
                    std::cout << message << std::endl;
                }
            }

            continue;
        }

        if (bytesRead == 0) {
            // Wszystkie konce zapisujace potoku zostaly zamkniete
            break;
        }

        if (errno == EINTR) {
            continue;
        }

        std::cerr << "Blad read(): " << std::strerror(errno) << '\n';
        break;
    }

    // Obsluga ewentualnego ostatniego komunikatu bez znaku nowej linii
    if (!pending.empty()) {
        std::cout << pending << std::endl;
    }

    close(pipeFd[0]);

    // Oczekiwanie na zakonczenie wszystkich uruchomionych procesow
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

    std::cout << "Wszystkie mrowki zakonczyly dzialanie.\n";

    return 0;
}
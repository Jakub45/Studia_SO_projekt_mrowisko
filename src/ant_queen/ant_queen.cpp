#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <limits.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <poll.h>
#include <stdio.h>

#define MSG_BUF_SIZE 256

#include "../common/error_handler/error_handler.hpp"


struct EggData
{
    bool is_active;
    int id;
    struct timespec birth_time;
};


// Zwraca katalog, w ktorym lezy uruchomiony program
std::string ExeDir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);

    //obsluga bledu
    if (len == -1) {
        Error(ErrorType::ERROR_READ, ErrorSource::SOURCE_ANT_QUEEN, 
        "Blad odczytu sciezki programu", errno);
        return ".";
    }
    path[len] = '\0';

    std::string s(path);
    return s.substr(0, s.find_last_of('/'));
}


pid_t Ant_born(int i, int antWriteFd) {
    pid_t pid = fork();

    //obsluga bledu
    if (pid == -1) {
        Error(ErrorType::ERROR_FORK, ErrorSource::SOURCE_ANT_QUEEN, 
        "blad utworzenia procesu ant",errno);
        return -1;
    }

    if (pid == 0) {

        //obsluga bledu
        if (dup2(antWriteFd, STDOUT_FILENO) == -1) {
            Error(ErrorType::ERROR_DUP2, ErrorSource::SOURCE_ANT_QUEEN, 
            "Blad przekierowania STDOUT do potoku Ant", errno);
            _exit(static_cast<int>(ErrorType::ERROR_DUP2));
        }

        close(antWriteFd);

        std::string antId = std::to_string(i);
        std::string antPath = ExeDir() + "/ant";

        execl(antPath.c_str(),
              "ant",
              antId.c_str(),
              static_cast<char*>(nullptr));

        //obsluga bledu
        Error(ErrorType::ERROR_EXEC, ErrorSource::SOURCE_ANT_QUEEN,
        "Blad uruchomienia programu Ant", errno);
        _exit(static_cast<int>(ErrorType::ERROR_EXEC));
    }

    return pid;
}




//proces ant_queen dostaje:
//ilosc mrowek,  (zmienna population)
//pojemnosc wylegarni, (zmienna granary_volume)
//ilosc jedzenia,  (zmienna food)
//potok do mrowek,
//komendy, 

//proces ant_queen wysyla:
//informacje o ilosci jaj,     (zmienna eggs)
//informacje o wykluciu mrowki
//informacje o ilosci jedzenia  (zmienna food)


EggData egg_incubator(int x) {
    struct timespec ts;
    EggData egg;
    egg.id = x;
    egg.is_active = true;
    clock_gettime(CLOCK_MONOTONIC, &egg.birth_time);

    return egg;
}



// Czyta z deskryptora jedna linie (do '\n') do tablicy line.
// buffer + buf_len przechowuja niepelne dane miedzy wywolaniami.
// Zwraca: 1 = odebrano linie, 0 = EOF, -1 = blad
int read_line(int fd, char* buffer, int* buf_len, char* line, int line_size) {
    while (1) {
        // sprawdz, czy w buforze jest juz cala linia
        char* nl = (char*)memchr(buffer, '\n', *buf_len);
        if (nl != NULL) {
            int len = nl - buffer;
            if (len >= line_size) {
                len = line_size - 1;
            }
            memcpy(line, buffer, len);
            line[len] = '\0';

            // usun przetworzona linie z bufora
            int rest = *buf_len - (nl - buffer) - 1;
            memmove(buffer, nl + 1, rest);
            *buf_len = rest;
            return 1;
        }

        if (*buf_len >= MSG_BUF_SIZE) {
            *buf_len = 0;   // bufor pelny bez '\n' - odrzuc smieci
            Error(ErrorType::ERROR_BUFFER_OVERFLOW, ErrorSource::SOURCE_ANT_QUEEN, 
            "Przepelnienie bufora podczas odbierania danych. Program dalej dziala", 0);
        }

        ssize_t bytes = read(fd, buffer + *buf_len, MSG_BUF_SIZE - *buf_len);

        if (bytes > 0) {
            *buf_len += bytes;
        }
        else if (bytes == 0) {
            return 0;   // EOF
        }
        else if (errno == EINTR) {
            continue;
        }
        else {
            //obsluga bledu
            Error(ErrorType::ERROR_READ, ErrorSource::SOURCE_ANT_QUEEN, 
            "Blad odczytu danych z potoku", errno);
            return -1;
        }
    }
}

// Rozbija linie na tokeny i zamienia je na liczby przez atoi.
// Zwraca 1 jesli udalo sie odczytac wszystkie 4 wartosci, inaczej 0.
int parse_message(char* line, int* population, int* granary_volume,
                  int* food, int* max_population) {
    int* targets[4] = { population, granary_volume, food, max_population };

    char* token = strtok(line, " ");
    for (int i = 0; i < 4; i++) {
        if (token == NULL) {
            Error(ErrorType::ERROR_PARSE, ErrorSource::SOURCE_ANT_QUEEN,
            "Za malo wartosci w odebranym komunikacie", 0);
            return 0;
        }
        *targets[i] = atoi(token);
        token = strtok(NULL, " ");
    }
    return 1;
}




bool egg_ready(EggData egg, int incubation_time) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == -1) {
        Error(
            ErrorType::ERROR_CLOCK,
            ErrorSource::SOURCE_ANT_QUEEN,
            "Blad funkcji czasu dla jajek",
            errno
        );
        return false;
    }
    time_t elapsed = now.tv_sec - egg.birth_time.tv_sec;
    return elapsed >= incubation_time;
}

void send_to_anthill(int fd, const char* text) {
    size_t len = strlen(text);
    size_t sent = 0;

    while (sent < len) {
        ssize_t bytes = write(fd, text + sent, len - sent);

        if (bytes > 0) {
            sent += bytes;
        }
        else if (bytes == -1 && errno == EINTR) {
            continue;
        }
        else {
            Error(ErrorType::ERROR_WRITE, ErrorSource::SOURCE_ANT_QUEEN,
                "Blad wysylania danych do main", bytes == -1 ? errno : 0);
            return;
        }
    }
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        Error(ErrorType::ERROR_ARGUMENT, ErrorSource::SOURCE_ANT_QUEEN,
        "Blad przekazywanych danych z main do Ant_Queen",errno);
        return 1;
    }
    int queenReadFd = atoi(argv[1]);
    int antWriteFd = atoi(argv[2]);

    char buffer[MSG_BUF_SIZE];
    int buf_len = 0;
    char line[MSG_BUF_SIZE];

    int population = 0;
    int granary_volume = 0;
    int food = 0;
    int max_population = 0;
    int num_of_egg = 11;

    std::vector<EggData> egg_data;

    bool running = true;
    while (running) {

        // czekaj maksymalnie 500 ms na dane od main
        struct pollfd pfd;
        pfd.fd = queenReadFd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int ret = poll(&pfd, 1, 500);

        if (ret > 0) {
            // main cos przyslal - odbierz i rozbij na zmienne
            int r = read_line(queenReadFd, buffer, &buf_len, line, sizeof(line));
            if (r == 1) {
                if (!parse_message(line, &population, &granary_volume, &food, &max_population)) {
                }
            }
            else if (r == 0) {
                running = false;   // main zamknal potok
            }

        }
        else if (ret == 0) {
            // cisza przez 500 ms - daj znac main, ze zyjemy
            // (main odpowie wtedy swoim stanem)
            send_to_anthill(antWriteFd, "QUEEN_ALIVE\n");
        }
        else if (ret == -1) {
            if (errno == EINTR) {
                continue;
            }

            Error(
                ErrorType::ERROR_POLL,
                ErrorSource::SOURCE_ANT_QUEEN,
                "Blad oczekiwania na dane z potoku",
                errno
            );
            continue;
        }

        // zlozenie jaja: jest miejsce w wylegarni, miejsce w populacji i jedzenie
        if ((int)egg_data.size() < granary_volume
            && population < max_population
            && food >= 10) {
            egg_data.push_back(egg_incubator(num_of_egg));
            num_of_egg++;
            food = food - 10;
            send_to_anthill(antWriteFd, "EGG_LAID\n");
        }

        // sprawdzenie, czy jakies jajo sie wykluło
        for (size_t i = 0; i < egg_data.size(); ) {
            if (egg_ready(egg_data[i], 30)) {
                pid_t antPid = Ant_born(egg_data[i].id, antWriteFd);
                if (antPid != -1) {
                    egg_data.erase(egg_data.begin() + i);
                    send_to_anthill(antWriteFd, "EGG_HATCHED\n");
                }
                else {
                    i++;
                }
            }
            else {
                i++;
            }
        }

        // posprzataj zakonczone procesy mrowek (zeby nie zostawaly zombie)
        pid_t result = waitpid(-1, NULL, WNOHANG);

        if (result == -1 && errno != EINTR && errno != ECHILD) {
            Error(ErrorType::ERROR_WAITPID, ErrorSource::SOURCE_ANT_QUEEN,
                "Blad waitpid czyszczacy procesy mrowek", errno );
        }   
    }
    return 0;
}
#include <iostream>
#include <string>
#include <vector>
#include <cerrno>
#include <cstring>
#include <limits.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "../common/error_handler/error_handler.hpp"

#include <cstdio>
#include <ctime>
#include <initializer_list>

// Zwraca katalog, w ktorym lezy uruchomiony program
std::string ExeDir() {
    char path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);

    //obsluga bledu
    if (len == -1) {
        Error(ErrorType::ERROR_READ, ErrorSource::SOURCE_MAIN_ANTHILL, 
        "Blad odczytu sciezki programu");
        return ".";
    }
    path[len] = '\0';

    std::string s(path);
    return s.substr(0, s.find_last_of('/'));
}

//---------------------------( Tworzenie procesu TUI )----------------//
pid_t StartTui(int readFd, int writeFd) {
    pid_t pid = fork();

    //obsluga bledu
    if (pid == -1) { 
        Error(ErrorType::ERROR_FORK, ErrorSource::SOURCE_MAIN_ANTHILL, 
        "Blad uruchomienia procesu TUI"); 
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

        //obsluga bledu
        Error(ErrorType::ERROR_EXEC, ErrorSource::SOURCE_MAIN_ANTHILL, 
        "Blad uruchamiania TUI");
        _exit(1);
    }

    return pid;
}




//--------------------( Tworzenie procesu mrowki )--------------------//
pid_t Ant_born(int i, int antPipe[2]) {
    pid_t pid = fork();

    //obsluga bledu
    if (pid == -1) { 
        Error(ErrorType::ERROR_FORK, ErrorSource::SOURCE_MAIN_ANTHILL, 
        "Blad tworzenia procesu Ant"); 
        return -1;
    }

    
    if (pid == 0) {
        close(antPipe[0]);

        //obsluga bledu
        if (dup2(antPipe[1], STDOUT_FILENO) == -1) {
            Error(ErrorType::ERROR_DUP2, ErrorSource::SOURCE_MAIN_ANTHILL,
            "Blad przekierowania STDOUT procesu Ant");
            _exit(static_cast<int>(ErrorType::ERROR_DUP2));
        }

        close(antPipe[1]);
        std::string antId = std::to_string(i);
        std::string antPath = ExeDir() + "/ant";
        execl(antPath.c_str(),
              "ant",
              antId.c_str(),
              static_cast<char*>(nullptr));


        //obsluga bledu
        Error(ErrorType::ERROR_EXEC, ErrorSource::SOURCE_MAIN_ANTHILL,
        "Blad uruchomienia pliku wykonywalnego ant");
        _exit(static_cast<int>(ErrorType::ERROR_EXEC));
    }
    return pid;
}

//----------------------(tworzenie procesu krolowej)-------------------//
pid_t Queen_born(int readFd, int writeFd, int antWriteFd, int antPipeReadFd) {
    pid_t pid = fork();

    //obsluga bledu
    if (pid == -1) { 
        Error(ErrorType::ERROR_FORK, ErrorSource::SOURCE_MAIN_ANTHILL, 
        "Blad uruchomienia procesu Queen"); 
        return -1;
    }

    if (pid == 0) {
        close(writeFd);
        close(antPipeReadFd);
        std::string fd = std::to_string(readFd);
        std::string antFd = std::to_string(antWriteFd);

        std::string path = ExeDir() + "/ant_queen";

        execl(path.c_str(),
              "ant_queen",
              fd.c_str(),
              antFd.c_str(),
              static_cast<char*>(nullptr));

        
        Error(ErrorType::ERROR_EXEC, ErrorSource::SOURCE_MAIN_ANTHILL,
        "Blad uruchomienia ant_queen");
        _exit(1);
    }

    return pid;
}


//-----------------------( Tworzenie potoku )-------------------------//
int CreatePipe(int (&pipeFd)[2]) {
    //obsluga bledu
    if (pipe(pipeFd) == -1) {
        Error(ErrorType::ERROR_PIPE, ErrorSource::SOURCE_MAIN_ANTHILL,
        "blad utworzenia pipe");
        return -1;
    }
    return 0;
}

void message_to_queen(int population, int granary_volume,
                      int food, int max_population, int queenWriteFd) {

    std::string message =
        std::to_string(population) + " " +
        std::to_string(granary_volume) + " " +
        std::to_string(food) + " " +
        std::to_string(max_population) + "\n";

    size_t sent = 0;

    while (sent < message.size()) {
        ssize_t bytes = write(
            queenWriteFd,
            message.data() + sent,
            message.size() - sent
        );

        if (bytes > 0) {
            sent += bytes;
        }
        else if (bytes == -1 && errno == EINTR) {
            continue;
        }

        else {
            //obsluga bledu
            Error(ErrorType::ERROR_WRITE, ErrorSource::SOURCE_MAIN_ANTHILL,
            "Blad wysylania danych do procesu queen");
            break;
        }
    }
}
//---------------(funkcja przekazujaca wartosci condition do tui)------------------//
void condition_to_tui(int tuiWriteFd, int queen, int pop,
                 int gran, int food, int eggs, int tun) {
    std::string message =
        "CONDITION " + 
        std::to_string(queen) + " " +
        std::to_string(pop) + " " +
        std::to_string(gran) + " " +
        std::to_string(food) + " " +
        std::to_string(eggs) + " " +
        std::to_string(tun) + "\n";

    std::size_t sent = 0;

    while (sent < message.size()) {
        ssize_t bytesWritten = write(
            tuiWriteFd,
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
            //obsluga bledu
            Error(ErrorType::ERROR_WRITE, ErrorSource::SOURCE_MAIN_ANTHILL,
            "Blad wysylania danych condition do TUI");
            break;
        }
    }
}

//---------------(funkcja przekazujaca wartosci event do tui)------------------//
void event_to_tui(int tuiWriteFd, const char* buffer,
                  std::size_t bytesRead, std::string& pending) {
    pending.append(buffer, bytesRead);

    std::size_t pos;

    while ((pos = pending.find('\n')) != std::string::npos) {
        std::string message = "EVENT " + pending.substr(0, pos + 1);
        pending.erase(0, pos + 1);

        std::size_t sent = 0;

        while (sent < message.size()) {
            ssize_t bytesWritten = write(
                tuiWriteFd,
                message.data() + sent,
                message.size() - sent
            );

            if (bytesWritten > 0) {
                sent += static_cast<std::size_t>(bytesWritten);
            } else if (bytesWritten == -1 && errno == EINTR) {
                continue;
            } else {
                //obsluga bledu
                Error(ErrorType::ERROR_WRITE, ErrorSource::SOURCE_MAIN_ANTHILL,
                "Blad wysylania danych event do TUI");
                break;
            }
        }
    }
}

//funkcje zapisu do pliku logow
//lista inicjalizujaca pozwala zapisac dowolna ilosc ciagow znakow
void saveLogs(FILE* fileLog, const std::string& text) {
    time_t t = time(NULL);
    struct tm* time = localtime(&t);

    if (fileLog != NULL && time != NULL) {
        fprintf(fileLog, "[%02d:%02d:%02d] : [%s] \n",
            time->tm_hour,
            time->tm_min,
            time->tm_sec,
            text.c_str() //funkcja c++ do zapisu ciagu znakow
        );
        fflush(fileLog);
    }
}
//zapis stopki do logu
void saveFooter(FILE* fileLog){
    if (fileLog != NULL) {
        fprintf(fileLog,
            "=============================NEW SIMULATION===============================\n"
        );
        fflush(fileLog);
    }
}


//--------------------------( MAIN )------------------------------------//
int main() {
    // zmienne
    int max_population = 300;
    int start = 10;

    bool is_queen_alive = 0;
    int population = 0;
    int granary_volume = 100;
    int food = 100;
    int eggs = 0;
    int num_of_tun = 3;

    int antPipe[2];
    int tuiPipe[2];
    int queenPipe[2];



    //------------otwarcie pliku do zapisu logow----------------//
    time_t t = time(NULL);
    struct tm* czas = localtime(&t);
    FILE* fileLog = fopen("../logs/logs.log", "a");
    //obsluga bledu
    if (fileLog == NULL){
        Error(ErrorType::ERROR_FILE_NOT_FOUND, ErrorSource::SOURCE_MAIN_ANTHILL, 
        "blad otwarcia pliku logs.log", errno);
    }
    saveFooter(fileLog); //zapis stopki



    //------------Tworzenie potokow komunikacji-----------------//
    // Utworzenie potoku Ant
    //obsluga bledu
    if (CreatePipe(antPipe) != 0) {
        return -1;
    }
    // Utworzenie potoku do TUI
    //obsluga bledu
    if (CreatePipe(tuiPipe) != 0) {
        close(antPipe[0]);
        close(antPipe[1]);
        return -1;
    }
    // Utworzenie potoku do Queen
    //obsluga bledu
    if (CreatePipe(queenPipe) != 0) {
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
        return -1;
    }


    // Uruchomienie TUI z koncem odczytujacym tuiPipe
    pid_t tuiPid = StartTui(tuiPipe[0], tuiPipe[1]);
    //obsluga bledu
    if (tuiPid == -1) {
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
    }


    // main nie potrzebuje wlasnej kopii konca odczytujacego TUI
    close(tuiPipe[0]);
    std::vector<pid_t> processes;

    // Utworzenie procesu krolowej
    pid_t queenPid = Queen_born(
        queenPipe[0],
        queenPipe[1],
        antPipe[1],
        antPipe[0]
    );
    if (queenPid == -1) {
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
        close(queenPipe[0]);
        close(queenPipe[1]);
    }
    else{
        is_queen_alive = true;
    }
    // Utworzenie poczatkowych procesow mrowek w ilosci start
    for (int i = 1; i <= start; ++i) {
        pid_t pid = Ant_born(i, antPipe);
        if (pid == -1) {
            continue;
        }
        processes.push_back(pid);
    }


    // main nie zapisuje bezposrednio do antPipe
    close(antPipe[1]);
    char buffer[256];
    std::string pending;


    while (true) {
        ssize_t bytesRead = read(antPipe[0], buffer, sizeof(buffer) -1);
        message_to_queen(
            population,
            granary_volume,
            food,
            max_population,
            queenPipe[1]
        );
        if (bytesRead > 0) {
            pending.append(buffer, bytesRead);

            std::size_t pos;

            while ((pos = pending.find('\n')) != std::string::npos) {
                std::string event = pending.substr(0, pos);
                pending.erase(0, pos + 1);

                // type = pierwsze slowo z TEJ linii
                std::string type = event.substr(0, event.find(' '));

                if (type == "ANT_BORN") {
                    population++;
                }
                else if (type == "FOOD_FOUND") {
                    food++;
                }
                else if (type == "ANT_DEAD") {
                    population--;
                }
                else if (type == "EGG_LAID") {
                    eggs++;
                    food = food - 10;
                }
                else if (type == "EGG_HATCHED") {
                    eggs--;
                }
                else if (type == "QUEEN_ALIVE") {
                    continue;   // tylko budzi main, nic wiecej
}

                std::string message = event + "\n";
                std::string eventPending;

                event_to_tui(tuiPipe[1], message.c_str(),
                            message.size(), eventPending);
                saveLogs(fileLog, event);
                condition_to_tui(tuiPipe[1], is_queen_alive, population,
                                granary_volume, food, eggs, num_of_tun);
            }
            continue;
        }
        if (bytesRead == 0) {
            break;
        }
        if (errno == EINTR) {
            continue;
        }
        //obsluga bledu
        Error(ErrorType::ERROR_READ, ErrorSource::SOURCE_MAIN_ANTHILL,
        "Blad read z antPipe");
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
                //obsluga bledu
                Error(ErrorType::ERROR_WRITE, ErrorSource::SOURCE_MAIN_ANTHILL,
                "Blad przekazywania ostatniego komunikatu do TUI");
            }
        }
    }
    close(antPipe[0]);


    // Koniec danych dla TUI
    close(tuiPipe[1]);
    // Oczekiwanie na zakonczenie mrowek
    for (pid_t pid : processes) {
        int childStatus;

        while (waitpid(pid, &childStatus, 0) == -1) {
            if (errno == EINTR) {
                continue;
            }
            //obsluga bledu
            Error(ErrorType::ERROR_WAITPID, ErrorSource::SOURCE_MAIN_ANTHILL,
            "Blad waitpid dla Ant");
            break;
        }
    }
    // Oczekiwanie na zakonczenie TUI
    int tuiStatus;

    while (waitpid(tuiPid, &tuiStatus, 0) == -1) {
        if (errno == EINTR) {
            continue;
        }
        //obsluga bledu
        Error(ErrorType::ERROR_WAITPID, ErrorSource::SOURCE_MAIN_ANTHILL,
        "Blad waitpid dla TUI");
        break;
    }


    std::cout << "Wszystkie procesy zakonczone.\n";

    fclose(fileLog);
    return 0;
}
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

#include <poll.h>


std::string ExeDir();

int CreatePipe(int (&pipeFd)[2]);

pid_t Queen_born(int readFd, int writeFd, int antWriteFd, int antPipeReadFd, int outReadFd, int outWriteFd);
pid_t Ant_born(int i, int antPipe[2]);
pid_t StartTui(int readFd, int writeFd, int commandReadFd, int commandWriteFd);


void saveLogs(FILE* fileLog, const std::string& text);
void saveFooter(FILE* fileLog);

void event_to_tui(int tuiWriteFd, const std::string& event);

void condition_to_tui(int tuiWriteFd, int queen, int pop, int gran, int food, int eggs, int tun);
void message_to_queen(int population, int granary_volume, int food, int max_population, int queenWriteFd, int laid_seen);

std::string messageFromQueen(char buffer[256], std::string& pending, ssize_t bytesRead, int& food, int& eggs,int& laid_seen);
std::string messageFromAnt(char buffer[256], std::string& pending, ssize_t bytesRead, int& population, int& food);

void messageFromTui(char buffer[256],std::string& pending, ssize_t bytesRead, bool& running);




//--------------------------( MAIN )------------------------------------//
int main() {
    // zmienne
    int max_population = 300;
    int start = 10;

    bool is_queen_alive = 0;
    int population = 0;
    int granary_volume = 100;
    int food = 0;
    int eggs = 0;
    int num_of_tun = 3;

    int antPipe[2];

    int tuiPipe[2];
    int tuiToMain[2];

    int queenPipe[2];
    int queenOutPipe[2];

    bool running = true;
    int pollResult;
    int laid_seen = 0;   // ile EGG_LAID main juz przetworzyl



    //------------otwarcie pliku do zapisu logow----------------//
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
    // Utworzenie potoku z TUI
    //obsluga bledu
    if (CreatePipe(tuiToMain) != 0) {
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
        return -1;
    }
    // Utworzenie potoku do Queen
    //obsluga bledu
    if (CreatePipe(queenPipe) != 0) {
        close(tuiToMain[0]);
        close(tuiToMain[1]);
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
        return -1;
    }
    if (CreatePipe(queenOutPipe) != 0) {
        close(queenPipe[0]);
        close(queenPipe[1]);
        close(tuiToMain[0]);
        close(tuiToMain[1]);
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
        return -1;

}


    // Uruchomienie TUI z koncem odczytujacym tuiPipe
    pid_t tuiPid = StartTui(tuiPipe[0],tuiPipe[1],tuiToMain[0],tuiToMain[1]);
    //obsluga bledu
    if (tuiPid == -1) {
        close(antPipe[0]);
        close(antPipe[1]);
        close(tuiPipe[0]);
        close(tuiPipe[1]);
    }

    // main nie potrzebuje wlasnej kopii konca odczytujacego TUI
    close(tuiPipe[0]);
    close(tuiToMain[1]);

    std::vector<pid_t> processes;

    // Utworzenie procesu krolowej
    pid_t queenPid = Queen_born(queenPipe[0], queenPipe[1], antPipe[1], antPipe[0], queenOutPipe[0], queenOutPipe[1]);
    close(queenPipe[0]);
    close(queenOutPipe[1]);   // main tylko czyta od krolowej
    
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

    //bufory
    char bufferAnt[256];
    char bufferQueen[256];
    char bufferTui[256];
    char bufferEnvironment[256];
    
    std::string pendingAnt;
    std::string pendingQueen;
    std::string pendingTui;
    std::string pendingEnvironment;

    //glowna petla programu

    while (running) {
        std::string messageAnt;
        std::string messageQueen;

        struct pollfd potoki[3];

        potoki[0].fd = antPipe[0];
        potoki[0].events = POLLIN;

        potoki[1].fd = queenOutPipe[0];
        potoki[1].events = POLLIN;

        potoki[2].fd = tuiToMain[0];
        potoki[2].events = POLLIN;

        // Wiadomosc do krolowej
        message_to_queen(population, granary_volume, food, max_population, queenPipe[1], laid_seen);

        // Aktualizacja stanu do TUI
        condition_to_tui(tuiPipe[1], is_queen_alive, population, granary_volume, food, eggs, num_of_tun);



        do {
            pollResult = poll(potoki, 3, -1);
        } while (pollResult == -1 && errno == EINTR);

        if (pollResult == -1) {
            Error(ErrorType::ERROR_POLL, ErrorSource::SOURCE_MAIN_ANTHILL,
                "Blad funkcji poll",errno);
        }

        for (int i = 0; i < 3; i++) {
            if (potoki[i].revents & POLLIN) {
                switch (i) {
                    //wiadomosc od Ant
                    case 0: {
                        ssize_t bytesReadAnt = read(antPipe[0], bufferAnt, sizeof(bufferAnt) - 1);
                        if (bytesReadAnt > 0) {
                            messageAnt = messageFromAnt(bufferAnt, pendingAnt, bytesReadAnt, population, food);
                        }
                        break;
                    }


                    // Wiadomosc od krolowej
                    case 1: {
                        ssize_t bytesReadQueen = read(queenOutPipe[0], bufferQueen, sizeof(bufferQueen) - 1 );
                        if (bytesReadQueen > 0) {
                            messageQueen = messageFromQueen(bufferQueen,pendingQueen,bytesReadQueen,food,eggs,laid_seen);
                        }
                        break;
                    }

                    // wiadomosc od TUI
                    case 2: {
                        ssize_t bytesReadTui = read(tuiToMain[0], bufferTui, sizeof(bufferTui) - 1);
                        if (bytesReadTui > 0) {
                            messageFromTui(bufferTui, pendingTui, bytesReadTui, running) ;
                        }
                        break;
                    }   
                }
            }
        }
        // Polaczenie komunikatow
        std::string message = messageQueen + messageAnt;

        // Przekazanie zdarzen do TUI
        std::size_t start = 0;
        std::size_t end;
        while ((end = message.find('\n', start)) != std::string::npos) {
            std::string event = message.substr(start, end - start);
            start = end + 1;

            if (event.empty()) {
                continue;
            }

            event_to_tui(tuiPipe[1], event);
            saveLogs(fileLog, event);
        }
    }

    



    // Przekazanie ostatniego komunikatu bez znaku nowej linii
    if (!pendingAnt.empty()) {
        std::size_t sent = 0;

        while (sent < pendingAnt.size()) {
            ssize_t bytesWritten = write(
                tuiPipe[1],
                pendingAnt.data() + sent,
                pendingAnt.size() - sent
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
    close(tuiToMain[0]);
    close(queenPipe[1]);
    close(queenOutPipe[0]);
    close(antPipe[0]);
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




//----------------------(tworzenie procesu krolowej)-------------------//
pid_t Queen_born(int readFd, int writeFd, int antWriteFd, int antPipeReadFd,int outReadFd, int outWriteFd) {
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
        close(outReadFd);
        std::string fd = std::to_string(readFd);
        std::string antFd = std::to_string(antWriteFd);
        std::string outFd = std::to_string(outWriteFd);

        std::string path = ExeDir() + "/ant_queen";

        execl(path.c_str(), "ant_queen", fd.c_str(), antFd.c_str(), outFd.c_str(), static_cast<char*>(nullptr));
        
        Error(ErrorType::ERROR_EXEC, ErrorSource::SOURCE_MAIN_ANTHILL,
        "Blad uruchomienia ant_queen");
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


//---------------------------( Tworzenie procesu TUI )----------------//
pid_t StartTui(int readFd, int writeFd, int commandReadFd, int commandWriteFd) {
    pid_t pid = fork();

    if (pid == -1) {
        Error(ErrorType::ERROR_FORK,
              ErrorSource::SOURCE_MAIN_ANTHILL,
              "Blad uruchomienia procesu TUI");
        return -1;
    }

    if (pid == 0) {
        close(writeFd);
        close(commandReadFd);

        std::string fdRead = std::to_string(readFd);
        std::string fdWrite = std::to_string(commandWriteFd);
        std::string path = ExeDir() + "/tui";

        execl(path.c_str(),
              "tui",
              fdRead.c_str(),
              fdWrite.c_str(),
              static_cast<char*>(nullptr));

        Error(ErrorType::ERROR_EXEC,ErrorSource::SOURCE_MAIN_ANTHILL,
              "Blad uruchamiania TUI");

        _exit(1);
    }

    return pid;
}


std::string messageFromQueen(char buffer[256], std::string& pending, ssize_t bytesRead, int& food, int& eggs, int& laid_seen){
    std::string events;

    pending.append(buffer, bytesRead);
    std::size_t pos;

    while ((pos = pending.find('\n')) != std::string::npos) {
        std::string event = pending.substr(0, pos);
        pending.erase(0, pos + 1);

        std::string type = event.substr(0, event.find(' '));

        if (type == "QUEEN_ALIVE") {
            continue;   // nie idzie do TUI ani do logu
        }
        else if (type == "EGG_LAID") {
            eggs++;
            laid_seen++;
            food -= 10;
        }
        else if (type == "EGG_HATCHED") {
            eggs--;
        }
        events += event + "\n";
    }

    return events;
}




std::string messageFromAnt(char buffer[256], std::string& pending, ssize_t bytesRead, int& population, int& food) {
    std::string events;

    pending.append(buffer, bytesRead);
    std::size_t pos;

    while ((pos = pending.find('\n')) != std::string::npos) {
        std::string event = pending.substr(0, pos);
        pending.erase(0, pos + 1);

        std::string type = event.substr(0, event.find(' '));

        // --- od mrowek ---
        if (type == "ANT_BORN") {
            population++;
        }
        else if (type == "ANT_DEAD") {
            population--;
        }
        else if (type == "FOOD_FOUND") {
            food++;
        }
        // --- od krolowej ---
        else if (type == "ANTS_ALIVE") {
            continue;   // tylko budzi main - nie idzie do TUI ani do logu
        }
        events += event + "\n";
    }

    return events;
}






void messageFromTui(char buffer[256],std::string& pending, ssize_t bytesRead, bool& running){
    pending.append(buffer, bytesRead);

    std::size_t pos;

    while ((pos = pending.find('\n')) != std::string::npos) {
        std::string event = pending.substr(0, pos);
        pending.erase(0, pos + 1);

        // type = pierwsze slowo z TEJ linii
        std::string typeTui = event.substr(0, event.find(' '));

        
        if (typeTui == "QUIT") {
            running = false;
            return;
        }
        else if (typeTui == "QUEEN_KILL") {
            //
        }
        else if (typeTui == "ANT_KILL") {
            int number;

            if (sscanf(event.c_str(), "ANT_KILL %d", &number) == 1) {

                //
            }
        }

        else if (typeTui == "ANTHILL_EXPANSION") {
            //
        }
        else if (typeTui == "TUNNEL_COLLAPSE") {
            //
        }
    }
}




//---------------(funkcja przekazujaca wartosci event do tui)------------------//
void event_to_tui(int tuiWriteFd, const std::string& event) {
    std::string message = "EVENT " + event + "\n";

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
            Error(ErrorType::ERROR_WRITE, ErrorSource::SOURCE_MAIN_ANTHILL,
            "Blad wysylania danych event do TUI");
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

void message_to_queen(int population, int granary_volume,
                      int food, int max_population, int queenWriteFd, int laid_seen) {

    std::string message =
        std::to_string(population) + " " +
        std::to_string(granary_volume) + " " +
        std::to_string(food) + " " +
        std::to_string(max_population) + " " +
        std::to_string(laid_seen) + "\n";
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

#include <ncurses.h> //tui

#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>
#include <cstring>  // strncpy, strtok
#include <iostream> //flush

#include <signal.h> //sigusr
volatile sig_atomic_t close_simulation = 0; //globalna flaga close (zamykanie symulacji) SIGUSR2

void sig_handler1(int sig){
    return;
    //pusty
}
void sig_handler2(int sig){
    //SIGUSR2
    close_simulation = 1;
}



//-( Wypisuje ostatnie "rows" linii z historii (starsze sie przewijaja). )-------//
void draw_lines(WINDOW* w, const std::vector<std::string>& lines, int rows) {
    int width = getmaxx(w);
    int start = 0;

    if ((int)lines.size() > rows) {
        start = (int)lines.size() - rows;
    }

    for (int i = start; i < (int)lines.size(); i++) {
        mvwprintw(w, i - start, 0, "%.*s", width, lines[i].c_str());
    }
}

void send_cmd(int fd, const std::string& msg) {
    size_t sent = 0;
    while (sent < msg.size()) {
        ssize_t n = write(fd, msg.data() + sent, msg.size() - sent);
        if (n > 0) sent += n;
        else if (n == -1 && errno == EINTR) continue;
        else break;
    }
}


//---------------------------------( MAIN )--------------------------------------//
int main(int argc, char* argv[]) {
    if (argc < 3) {
        return 1;
    }
    
    //SIGUSR1 i SIGUSR2
    struct sigaction sig1 = {0};
    struct sigaction sig2 = {0};
    sig1.sa_handler = sig_handler1;
    sig2.sa_handler = sig_handler2;
    sigemptyset(&sig1.sa_mask);
    sigemptyset(&sig2.sa_mask);
    sigaction(SIGUSR1, &sig1, NULL);
    sigaction(SIGUSR2, &sig2, NULL);



    int readFd = atoi(argv[1]);
    int cmdFd = atoi(argv[2]);

    // Pipe nieblokujacy, zeby read() nie zawieszal interfejsu.
    int flags = fcntl(readFd, F_GETFL, 0);
    fcntl(readFd, F_SETFL, flags | O_NONBLOCK);

    initscr();
    raw();
    noecho();

    // Okna glowne: newwin(wysokosc, szerokosc, y, x)
    WINDOW* win_event    = newwin(49, 60, 1, 1);
    WINDOW* win_info     = newwin(12, 60, 1, 62);
    WINDOW* win_kingdom  = newwin(8, 60, 13, 62);
    WINDOW* win_terminal = newwin(29, 60, 21, 62);

    if (!win_event || !win_info || !win_kingdom || !win_terminal) {
        endwin();
        return 1;
    }

    // Podokna na tresc (rozmiar okna minus ramka, pozycja 1,1 wzgledem rodzica).
    WINDOW* event_text    = derwin(win_event,    47, 58, 1, 1);
    WINDOW* info_text     = derwin(win_info,     10, 58, 1, 1);
    WINDOW* kingdom_text  = derwin(win_kingdom,   6, 58, 1, 1);
    WINDOW* terminal_text = derwin(win_terminal, 27, 58, 1, 1);

    if (!event_text || !info_text || !kingdom_text || !terminal_text) {
        endwin();
        return 1;
    }

    // Klawisze czytamy z okna terminal.
    keypad(terminal_text, TRUE);
    wtimeout(terminal_text, 100);

    char buffer[256];
    std::string received;

    std::vector<std::string> events;           // historia okna EVENT
    std::vector<std::string> terminal_lines;   // historia okna TERMINAL
    std::string input;                         // wpisywany tekst

    // Wartosci KINGDOM CONDITION (docelowo beda przychodzic z maina).
    bool queen_alive = true;
    int ant_population = 0;
    int granary_volume;
    int food;
    int eggs;
    int num_of_tunnels;

    int number;


    
    bool running = true;
    while (running) {


        // Odbierz dane z main (czytaj az do opróżnienia pipe'a).
        ssize_t n;
        while ((n = read(readFd, buffer, sizeof(buffer))) > 0) {
            received.append(buffer, n);
        }
        if (n == 0) {
            running = false;               // main zamknal pipe
        }
        // n == -1 z EAGAIN = brak danych, to normalne

        // Przetwarzaj kazda kompletna linie.
        std::size_t pos;
        while ((pos = received.find('\n')) != std::string::npos) {
            std::string line = received.substr(0, pos);
            received.erase(0, pos + 1);

            std::string type = line;
            std::string payload;

            std::size_t space = line.find(' ');
            if (space != std::string::npos) {
                type    = line.substr(0, space);
                payload = line.substr(space + 1);
            }

            if (type == "EVENT") {
                events.push_back(payload);
            }
            else if (type == "CONDITION") {
                char data[256];
                strncpy(data, payload.c_str(), sizeof(data) - 1);
                data[sizeof(data) - 1] = '\0';

                char* token = strtok(data, " ");
                if (token != NULL) queen_alive = atoi(token);

                token = strtok(NULL, " ");
                if (token != NULL) ant_population = atoi(token);

                token = strtok(NULL, " ");
                if (token != NULL) granary_volume = atoi(token);

                token = strtok(NULL, " ");
                if (token != NULL) food = atoi(token);

                token = strtok(NULL, " ");
                if (token != NULL) eggs = atoi(token);

                token = strtok(NULL, " ");
                if (token != NULL) num_of_tunnels = atoi(token);
            }
            else {
                terminal_lines.push_back(type + ": command not found");
            }
        }


        // Rysowanie.
        werase(win_event);
        werase(win_info);
        werase(win_kingdom);
        werase(win_terminal);

        box(win_event, 0, 0);
        box(win_info, 0, 0);
        box(win_kingdom, 0, 0);
        box(win_terminal, 0, 0);

        mvwprintw(win_event,    0, 2, " EVENT ");
        mvwprintw(win_info,     0, 2, " INFO ");
        mvwprintw(win_kingdom,  0, 2, " KINGDOM CONDITION ");
        mvwprintw(win_terminal, 0, 2, " TERMINAL ");

        // EVENT: cala historia.
        draw_lines(event_text, events, getmaxy(event_text));

        // INFO: staly tekst.
        mvwprintw(info_text, 0, 0, 
            "ANTHILL SIMULATION \n"
            "AVALIBLE COMMANDS:  \n"
            "quit              - wyjscie z aplikacji (poprawne). \n"
            "ctrl + Q          - wyjscie z aplikacji (awaryjne).\n"
            "queen_kill        - usmiercenie krolowej.\n"
            "ant_kill (number) - liczba mrowek do usmiercenia.\n"
            "anthill_expansion - Rozbudowa mrowiska (SIGUSR1).\n"
            "tunnel_collapse   - zawalenie losowego tunelu (SIGUSR2).\n"
        );

        // KINGDOM CONDITION
        mvwprintw(kingdom_text, 0, 0, "IS QUEEN ALIVE = %s",  queen_alive ? "true" : "false");
        mvwprintw(kingdom_text, 1, 0, "ANT POPULATION = %d",  ant_population);
        mvwprintw(kingdom_text, 2, 0, "GRANARY VOLUME = %d",  granary_volume);
        mvwprintw(kingdom_text, 3, 0, "FOOD = %d",            food);
        mvwprintw(kingdom_text, 4, 0, "EGGS = %d",            eggs);
        mvwprintw(kingdom_text, 5, 0, "NUM OF TUNNELS = %d",  num_of_tunnels);

        // TERMINAL: historia + linia wpisywania w ostatnim wierszu.
        int th = getmaxy(terminal_text);
        int tw = getmaxx(terminal_text);

        draw_lines(terminal_text, terminal_lines, th - 1);

        std::string prompt = "> " + input;
        if ((int)prompt.size() > tw - 1) {
            prompt = prompt.substr(prompt.size() - (tw - 1));
        }
        mvwprintw(terminal_text, th - 1, 0, "%s", prompt.c_str());
        wmove(terminal_text, th - 1, (int)prompt.size());

        wrefresh(win_event);
        wrefresh(win_info);
        wrefresh(win_kingdom);
        wrefresh(win_terminal);       // ostatni, zeby kursor zostal w terminalu



        //jesli zakonczenie zostanie zainicjowane
        if (close_simulation ){                
            terminal_lines.push_back("simulation closed for 5 seconds");
            werase(win_terminal);
            box(win_terminal, 0, 0);
            draw_lines(terminal_text, terminal_lines, getmaxy(terminal_text) - 1);
            wrefresh(win_terminal);

            sleep(5);
            running = false;
            break;
        }

        // Klawisze.
        int key = wgetch(terminal_text);

        if (key == 17) {                                    // Ctrl+Q
            running = false;
        }
        //komunikacja z main_anthill
        else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            if (!input.empty()) {
                terminal_lines.push_back("> " + input);
                if (input == "quit") {
                    send_cmd(cmdFd, "QUIT\n");
                }
                else if (input == "queen_kill") {
                    send_cmd(cmdFd, "QUEEN_KILL\n");
                }
                else if (sscanf(input.c_str(), "ant_kill %d", &number) == 1) {
                    send_cmd(cmdFd, "ANT_KILL " + std::to_string(number) + "\n");
                }
                else if (input == "anthill_expansion") {
                    send_cmd(cmdFd, "ANTHILL_EXPANSION\n");
                }
                else if (input == "tunnel_collapse") {
                    send_cmd(cmdFd, "TUNNEL_COLLAPSE\n");
                }
                else {
                    terminal_lines.push_back("command not found");
                }
                input = "";
            }
        }
        else if (key == KEY_BACKSPACE || key == 127 || key == 8) {
            if (!input.empty()) {
                input.erase(input.size() - 1);
            }
        }
        else if (key >= 32 && key <= 126) {
            input += (char)key;
        }
    }

    delwin(event_text);
    delwin(info_text);
    delwin(kingdom_text);
    delwin(terminal_text);

    delwin(win_event);
    delwin(win_info);
    delwin(win_kingdom);
    delwin(win_terminal);

    close(readFd);
    endwin();

    return 0;
}
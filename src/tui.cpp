#include <ncurses.h>

#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <string>
#include <vector>

// Wypisuje ostatnie "rows" linii z historii (starsze sie przewijaja).
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

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return 1;
    }

    int readFd = atoi(argv[1]);

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
    std::string queen_alive;
    std::string ant_population;
    std::string granary_volume;
    std::string food;
    std::string eggs;
    std::string num_of_tunnels;

    bool running = true;

    while (running) {
        // Odbierz dane z main.
        ssize_t n = read(readFd, buffer, sizeof(buffer));

        if (n > 0) {
            received.append(buffer, n);
        }
        else if (n == 0) {
            running = false;                   // main zamknal pipe
        }

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
            else if (type == "INFO") {
                // tekst w oknie INFO jest staly, wiec nic nie robimy
            }
            else {
                terminal_lines.push_back("command not found");
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
            "----------------------\n"
            "Program do symulacji mrowiska.\n\n"
            "AVALIBLE COMMANDS  \n"
            "CTRL + Q          - wyjscie z aplikacji.\n"
            "QUEEN_KILL        - usmiercenie krolowej.\n"
            "ANT_KILL (number) - liczba mrowek do usmiercenia.\n"
            "ANTHILL_EXPANSION - Rozbudowa mrowiska (SIGUSR1).\n"
            "TUNNEL_COLLAPSE   - zawalenie losowego tunelu (SIGUSR2).\n"
        );

        // KINGDOM CONDITION
        mvwprintw(kingdom_text, 0, 0, "IS QUEEN ALIVE = %s",  queen_alive.c_str());
        mvwprintw(kingdom_text, 1, 0, "ANT POPULATION = %s",  ant_population.c_str());
        mvwprintw(kingdom_text, 2, 0, "GRANARY VOLUME = %s",  granary_volume.c_str());
        mvwprintw(kingdom_text, 3, 0, "FOOD = %s",            food.c_str());
        mvwprintw(kingdom_text, 4, 0, "EGGS = %s",            eggs.c_str());
        mvwprintw(kingdom_text, 5, 0, "NUM OF TUNNELS = %s",  num_of_tunnels.c_str());

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

        // Klawisze.
        int key = wgetch(terminal_text);

        if (key == 17) {                                    // Ctrl+Q
            running = false;
        }
        else if (key == '\n' || key == '\r' || key == KEY_ENTER) {
            if (!input.empty()) {
                terminal_lines.push_back("> " + input);
                // Na razie zadna komenda nie jest znana.
                terminal_lines.push_back("command not found");
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
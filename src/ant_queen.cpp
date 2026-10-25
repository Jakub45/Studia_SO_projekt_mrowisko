#include <windows.h>
#include <iostream>
#include <string>

enum class Queen_status {
    QUEEN_BORN = 0,
    QUEEN_DEAD = 1,
};

struct Ant_Queen {
    Queen_status queen_status = Queen_status::QUEEN_BORN;
    int queen_lifetime;
};

int main() {

	return 0;
}
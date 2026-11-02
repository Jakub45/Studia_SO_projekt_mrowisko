#include <iostream>
#include <random>
#include <string>
#include <thread>
#include <chrono>

using namespace std;
using namespace std::this_thread;
using namespace std::chrono;

std::string ant_names[] = {
    "Paul", "Josh", "Billy", "John", "Mike", "Alex", "Jack", "Tom",
    "Harry", "Charlie", "Sam", "Ben", "Luke", "Ryan", "Jake", "Matt",
    "Nick", "Chris", "Adam", "David", "James", "Robert", "Kevin", "Brian",
    "Eric", "Mark", "Daniel", "Andrew", "Jason", "Brandon", "Tyler", "Dylan",
    "Ethan", "Noah", "Liam", "Oliver", "Oscar", "Henry", "George", "Leo",
    "Max", "Felix", "Simon", "Peter", "Scott", "Steve", "Tony", "Patrick",
    "Frank", "Bruce", "Kyle", "Sean", "Trevor", "Cody", "Jesse", "Aaron",
    "Nathan", "Caleb", "Connor", "Mason", "Logan", "Hunter", "Austin", "Blake",
    "Derek", "Garrett", "Jeremy", "Justin", "Keith", "Marcus", "Phillip", "Roger",
    "Spencer", "Zach", "Andy", "Bobby", "Danny", "Eddie", "Freddy", "Jimmy",
    "Johnny", "Kenny", "Larry", "Mickey", "Ricky", "Ronnie", "Sammy", "Terry",
    "Tommy", "Walter", "Wayne", "Alan", "Arthur", "Albert", "Arnold", "Bernard",
    "Calvin", "Clarence", "Curtis", "Dennis", "Douglas", "Edwin", "Eugene", "Gerald",
    "Gordon", "Harold", "Howard", "Jeffrey", "Leonard", "Martin", "Norman", "Raymond",
    "Russell", "Stanley", "Victor", "Vincent", "Wojtek", "Jakub", "Mateusz", "Kacper",
    "Bartek", "Tomek", "Michal", "Pawel", "Piotr", "Krzysiek", "Grzesiek", "Maciek",
    "Szymon", "Adrian", "Damian", "Dawid", "Filip", "Marcel", "Kuba", "Przemek",
    "Lukasz", "Rafal", "Sebastian", "Patryk", "Artur", "Norbert", "Igor", "Borys",
    "Franek", "Staszek", "Carlos", "Diego", "Miguel", "Javier", "Alejandro", "Kenji",
    "Hiro", "Akira", "Ren", "Yuki", "Takumi", "Satoshi", "Haruto", "Dmitri",
    "Viktor", "Nikolai", "Sergei", "Alexei", "Hans", "Klaus", "Otto", "Fritz",
    "Pierre", "Louis", "Hugo", "Marco", "Luca", "Enzo", "Andre", "Rafael",
    "Bruno", "Thiago", "Pablo", "Ghost", "Shadow", "Reaper", "Viper", "Falcon",
    "Wolf", "Raven", "Sniper", "Blazer", "Frost", "Venom", "Phantom", "Spectre",
    "Havoc", "Raptor", "Zero", "Nova", "Ace", "Clutch", "Pixel", "Nox",
    "Hex", "Zed", "Turbo", "Bonk", "Potato", "LagMaster", "Headshot", "Smoke",
    "Flash", "RushB", "EcoKing", "BotBarry", "Dusty", "Sneaky", "OneTap", "NoScope",
    "FragBoy", "Critical", "Wrench", "Mrowczyk", "AntMan", "QueenBee", "TinyTank", "Bugsy"
};

enum class Ant_status {
    ANT_BORN = 0,
    ANT_DEAD = 1,
    FOOD_FOUND = 2,
};


struct Ant {
    Ant_status ant_status = Ant_status::ANT_BORN;
    std::string ant_id;
    std::string ant_name;
    int ant_capacity = 10;
    int ant_lifetime = 2;
};

int random_generator(std::mt19937& gen, int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(gen);
}

std::string random_ant_name(std::mt19937& gen) {
    int count = sizeof(ant_names) / sizeof(ant_names[0]);
    int index = random_generator(gen, 0, count - 1);
    return ant_names[index];
}

int ant_foraging(std::mt19937& gen) {
    sleep_for(seconds(3));

    int k = random_generator(gen, 1, 200);

    if (k % 4 == 0) {
        return 1; // znaleziono pozywienie
    }

    return 0; // nie znaleziono pozywienia
}




int main(int argc, char* argv[]) {
    std::random_device rd;
    std::mt19937 gen(rd());

    if (argc < 2) {
        return 1;
    }

    Ant ant;
    ant.ant_id = argv[1];
    ant.ant_name = random_ant_name(gen);

    cout << "Mrowka nr " << ant.ant_id << ", " << ant.ant_name << ", rodzi sie!" << '\n';

    //poszukiwanie jedzenia az do smierci
    while (ant.ant_lifetime > 0) {
        int found = ant_foraging(gen);

        if (found == 1) {
            // Komunikat dla procesu nadrzednego przez stdout.
            cout << "FOOD_FOUND " << ant.ant_name << '\n' << flush;
            ant.ant_lifetime--;
        }
    }

    ant.ant_status = Ant_status::ANT_DEAD;
    return 0;
}
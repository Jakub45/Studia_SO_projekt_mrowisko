#include <stdio.h>
#include <windows.h>
#include <iostream>
#include <random>
#include <string>

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


enum class Ant_status
{
	dead = 0,
	live = 1,
	idle = 2
};


struct Ant
{
	Ant_status ant_status = Ant_status::live;
	char* ant_id;
    std::string ant_name;
	int ant_capacity = 10;
	int ant_lifetime = 10;
};

std::string random_ant_name() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    std::uniform_int_distribution<int> dist(
        0, sizeof(ant_names) / sizeof(ant_names[0]) - 1
    ); //wybor losowego imienia sposrod dostepnej tablicy

    return ant_names[dist(gen)];
}


int main(int argc, char* argv[]) {
    if (argc >= 2) {

        Ant ant;
        ant.ant_id = argv[1];
        ant.ant_name = random_ant_name();

        std::cout << "Mrowa numer:" << argv[1] << " imie mrowy: " << ant.ant_name << '\n';
    }
	return 0;
}
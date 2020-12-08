#include <iostream>
#include <fstream>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

using std::cout;
using std::endl;
using std::string;

void create_container(int argc, char *argv[]);
void start_container(int argc, char *argv[]);
void exec_command(int argc, char *argv[]);
void stop_container(int argc, char *argv[]);

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cout << "Usage: ./main COMMAND [OPTIONS]" << endl;
        return 0;
    }

    string command(argv[1]);

    if (command == "create") {
        create_container(argc, argv);
    }
}

void create_container(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Usage: ./main create CONTAINER_NAME IMAGE" << endl;
        exit(0);
    }

    string path = "containers/" + string(argv[2]);
    string overlay_upper_path = path + "/upper";
    string working_dir_path = path + "/work";

    mkdir(path.c_str(), 0755);
    mkdir(overlay_upper_path.c_str(), 0755);
    mkdir(working_dir_path.c_str(), 0755);

    string config_path = path + "/config.txt";    
    std::ofstream config_file(config_path);
    string image_path = "images/" + string(argv[3]);
    config_file << image_path;
}

void start_container(int argc, char *argv[]) {

}
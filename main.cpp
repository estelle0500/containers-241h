#include <iostream>
#include <fstream>
#include <string>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mount.h>

using std::cout;
using std::cerr;
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
    } else if (command == "start") {
        start_container(argc, argv);
    } else {
        cout << "Unrecognized command. Command must be one of: create start exec stop" << endl;
    }

    return 0;
}

void create_container(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Usage: ./main create CONTAINER_NAME IMAGE" << endl;
        exit(0);
    }

    string path = "containers/" + string(argv[2]);
    string mount_point = path + "/mnt";
    string image_path = "images/" + string(argv[3]);
    string overlay_upper_path = path + "/upper";
    string working_dir_path = path + "/work";

    mkdir(path.c_str(), 0755);
    mkdir(overlay_upper_path.c_str(), 0755);
    mkdir(working_dir_path.c_str(), 0755);
    mkdir(mount_point.c_str(), 0777);

    // Create a mount point using overlay filesystem
    string overlay_data = "lowerdir=" + image_path + ",upperdir=" + path + "/upper" 
                          + ",workdir=" + path + "/work";
    if (mount(image_path.c_str(), mount_point.c_str(), "overlay", 0, overlay_data.c_str()) != 0) {
        cerr << "Mount failed: " << strerror(errno) << endl;
        exit(1);
    }

    // Create a config file (no particular use right now)
    string config_path = path + "/config.txt";    
    std::ofstream config_file(config_path);
    config_file << image_path;
}

void start_container(int argc, char *argv[]) {
    if (argc != 3) {
        cout << "Usage: ./main start CONTAINER_NAME" << endl;
        exit(0);
    }
    
    int fork_code = fork();
    if (fork_code == -1) {
        cerr << "Could not start container, fork failed" << endl;
    } else if (fork_code != 0) {
        cout << "Started container on process " << fork_code << endl;
        return;
    }

    // Rest of the function will only be run by the child

    string path = "containers/" + string(argv[2]);
    string mount_point = path + "/mnt";
    chroot(mount_point.c_str());
    chdir("/");

    // TODO make namespace
    // TODO replace with a shell that will read commands from a pipe
    execl("/bin/ls", "ls", NULL);
}
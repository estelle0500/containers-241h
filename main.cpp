#include <iostream>
#include <fstream>
#include <string>

#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>

#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/wait.h>

#define CGROUP_FOLDER "/sys/fs/cgroup/"
#define concat(a,b) (a"" b)

using std::cout;
using std::cerr;
using std::endl;
using std::string;

void create_container(int argc, char *argv[]);
void start_container(int argc, char *argv[]);
void exec_command(int argc, char *argv[]);
void stop_container(int argc, char *argv[]);
void write_rule(const char* path, const char* value);

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

    wait(NULL);
    return 0;
}

void write_rule(const char* path, const char* value){
    int fp = open(path, O_WRONLY | O_APPEND);
    write(fp, value, strlen(value));
    close(fp);
}

void LimitProcesses(){
    mkdir(concat(CGROUP_FOLDER, "pids"), S_IRUSR | S_IWUSR);
    const char* pid = std::to_string((getpid())).c_str();
    write_rule(concat(CGROUP_FOLDER, "cgroup.procs"), pid);
    write_rule(concat(CGROUP_FOLDER, "pids.max"), "5");
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

void start_child(int argc, char *argv[]);
void start_parent(int child_pid);

void start_container(int argc, char *argv[]) {
    if (argc != 4) {
        cout << "Usage: ./main start CONTAINER_NAME COMMAND" << endl;
        exit(0);
    }
    
    unshare(CLONE_NEWCGROUP | CLONE_NEWNET | CLONE_NEWPID | CLONE_NEWNS);

    int fork_code = fork();
    if (fork_code == -1) {
        cerr << "Could not start container, fork failed" << endl;
    } else if (fork_code != 0) {
        cout << "Started container on process " << fork_code << endl;
        start_parent(fork_code);
    } else {
        start_child(argc, argv);
    }
}

void start_child(int argc, char *argv[]) {
    // Mount at overlay filesystem
    string path = "containers/" + string(argv[2]);
    string mount_point = path + "/mnt";

    if (chroot(mount_point.c_str()) == -1) {
        cerr << "chroot failed: " << strerror(errno) << endl;
        exit(1);
    }
    if (chdir("/") == -1) {
        cerr << "chdir failed: " << strerror(errno) << endl;
    }

    // Remount proc
    if (umount("/proc") == -1) {
        cerr << "umount proc failed: " << strerror(errno) << endl;
    }
    if (mount("proc", "/proc", "proc", 0, "") == -1) {
        cerr << "mount proc failed: " << strerror(errno) << endl;
    }

    // Make user namespace
    unshare(CLONE_NEWUSER);
    sched_yield();
    if (setuid(0) == -1) {
        cerr << "setuid failed: " << strerror(errno) << endl;
    }

    // Set network host
    string name = "myhostnamespace";
    if (sethostname(name.c_str(), name.length()) != 0) {
        cerr << "Failed to set host name." << endl;
        exit(-1);
    }

    // TODO replace with a shell that will read commands from a pipe
    if (execl("/bin/bash", "bash", "-c", argv[3], NULL) == -1) {
        cerr << "Exec failed: " << strerror(errno) << endl;
    }
}

void create_map(int child_pid, const char *file, const char *line) {
    char filename_buffer[100];
    snprintf(filename_buffer, 100, "/proc/%d/%s", child_pid, file);

    FILE *uid_map;
    if ((uid_map = fopen(filename_buffer, "w"))) {
        if (fprintf(uid_map, "%s", line) == -1) {
            cerr << "could not write to " << file << " file: " <<   strerror(errno) << endl;
        }
        fclose(uid_map);
    } else {
        cerr << "could not open" << file << " file: " << strerror(errno) << endl;
    }
}

void start_parent(int child_pid) {
    char map_buffer[100];
    snprintf(map_buffer, 100, "0 %d 100000\n", geteuid());

    create_map(child_pid, "uid_map", map_buffer);
    create_map(child_pid, "gid_map", map_buffer);
}
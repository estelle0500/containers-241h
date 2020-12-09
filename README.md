# Description
A minimal container app.

# Setup
Compile with `g++ main.cpp -o main`.
Set the sticky bit: `sudo chown root main; sudo chmod +s main`.
Run `./download_image.sh` to get an image.

# Sample use
```bash
sudo ./main create box1 rootfs
sudo ./main start box1
```
Currently this just prints the contents of the root directory in the container.

# Commands
```
./main create CONTAINER_NAME IMAGE
```
Creates a container inside the `containers/` folder, which will be backed by an image in `images/`.

```
./main start CONTAINER_NAME
```
Starts a process for the container, prints PID.

```
./main exec CONTAINER_NAME COMMAND
```
Executes a command in the shell of the specified container.

```
./main stop CONTAINER_NAME
```
Cleans up process.

# Features
## Implemented
1. Able to save/load container
2. Be able to have full read/write access to the container
3. Be able to have full write access to the container without polluting the real
filesystem
4. Container launches in a new root partition
5. Container moves in a new namespace
6. Have a user in the container that is not root
7. Have a user in the container that has sudo privileges
8. Be able to create a new container without being root (i.e. use the set-uid bit)
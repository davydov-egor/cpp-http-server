#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


int main() {

    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock == -1){
        std::cerr << "Socket error: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Socket created: " << sock << std::endl;


    close(sock);
    return 0;
}

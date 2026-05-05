#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>

void close_client(int sock){ //Закрытие сокета клиента и проверка закрытия
    int close_client_ret = close(sock); //Закрытие
    if(close_client_ret == -1){ //Проверка закрытия 
        std::cerr << "close(client_sock) error: " << strerror(errno) << std::endl;
    }
}

int main() {
    //Coздание сокета
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    //Исправление Address already in use
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    //Проверка сокета на ошибки
    if(sock == -1){
        std::cerr << "socket() error: " << strerror(errno) << std::endl;
        return 1;
    }

    std::cout << "Socket created: " << sock << std::endl;
    //Создание адреса
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8080);
    int pton_ret = inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
   //Проверка адреса на ошибки
    if(pton_ret == 0){
        std::cerr << "logic error: wrong IP\n";
        return 1;
    }
    if(pton_ret == -1){
        std::cerr << "inet_pton() error: " << strerror(errno) << std::endl;
        return 1;
    }
    //Привязка к адресу
    int bind_ret = bind(sock, (sockaddr*)&addr, sizeof(addr));
    //Проверка привязки
    if(bind_ret == -1){
        std::cerr << "bind() error: " << strerror(errno) << std::endl;
        return 1;
    }
    //Перевод сокета в режим ожидания
    int listen_ret = listen(sock, 5);
    //Проверка ошибок перевода
    if(listen_ret == -1){
        std::cerr << "listen() error: " << strerror(errno) << std::endl;
        return 1;
   }
    //Создание буферов
    char buffer[1024]; //Буфер запроса клиета
    sockaddr_in cl_addr; //Буфер адреса клиента
    socklen_t cl_addr_len = sizeof(cl_addr); //Длина адреса клиента
    char cl_IP[INET_ADDRSTRLEN]; //Адрес клиента
    
    do{
        //Прием подключения из очереди
        int client_sock = accept(sock, (sockaddr*)&cl_addr, (socklen_t*)&cl_addr_len);
        //Проверка ошибок приема
        if(client_sock == -1){
            std::cerr << "accept() error: " << strerror(errno) << std::endl;
            return 1;
        }
        do{
            //Чтение данных
            int bytes = recv(client_sock, buffer, sizeof(buffer), 0);
            //Проверка чтения данных
            if(bytes == 0){
                std::cerr << "Client disconnected\n";
                break;
            }
            if(bytes == -1){
                std::cerr << "recv() error: " << strerror(errno) << std::endl;
                break;
            }
            //Формирование ответа
            buffer[bytes] = '\0';
            std::string body(buffer, bytes);
            std::string resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "\r\n" +
            body;



            //Отправка ответа
            int write_ret = write(client_sock, resp.c_str(), resp.size());
            //Проверка отправки
            if(write_ret == -1){
                std::cerr << "write() error: " << strerror(errno) << std::endl;
                break;
            }
        }while(true);
        close_client(client_sock); 
    }while(true);


    //Закрытие сокета и проверка закрытия на ошибки
    int close_ret = close(sock);
    if(close_ret == -1){
        std::cerr << "close(sock) error: " << strerror(errno) << std::endl;
        return 1;
    }
    return 0;
}

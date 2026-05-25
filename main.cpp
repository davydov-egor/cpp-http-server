#include <iostream>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <unordered_map>

struct HTTP_request {
  std::string method;
  std::string target;
  std::string version;

  std::unordered_map<std::string, std::string> headers;

  std::string body;
};

std::string E400(){
  std::string resp =
  "HTTP/1.1 400 Bad Request\r\n"
  "Content-Type: text/plain\r\n"
  "Content-Length: 11\r\n"
  "\r\n"
  "Bad Request"; 
  return resp;
}



std::string E404(){
  std::string resp =
  "HTTP/1.1 404 Not Found\r\n"
  "Content-Type: text/plain\r\n"
  "Content-Length: 9\r\n"
  "\r\n"
  "Not Found"; 
  return resp;
}

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
  char raw_buffer[1024]; //Буферы запроса клиента
  std::string buffer; 
  sockaddr_in cl_addr; //Буфер адреса клиента
  socklen_t cl_addr_len = sizeof(cl_addr); //Длина адреса клиента
  char cl_IP[INET_ADDRSTRLEN]; //Адрес клиента
  HTTP_request req;
  std::string body; //Буфер тела ответа
  std::string type; //Буфер типа ответа
  std::string resp; //Буфер ответа
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
      int bytes = recv(client_sock, raw_buffer, sizeof(raw_buffer), 0);
      //Проверка чтения данных
      if(bytes == 0){
        std::cerr << "Client disconnected\n";
        break;
      }
      if(bytes == -1){
        std::cerr << "recv() error: " << strerror(errno) << std::endl;
        break;
      }
      //Передача значения
      buffer.assign(raw_buffer, bytes);
      //Очистка буфера ответа
      resp.clear();
      //Вывод запроса клиента
      std::cout << "Client request:\n" << buffer << std::endl;
      //Парсинг запроса
      size_t space1 = buffer.find(" "); //Поиск пробеллов
      size_t space2 = buffer.find(" ", space1 + 1);
      size_t h_start = 0;
      size_t colon = 0;
      size_t end = 0;
      if(space1 == std::string::npos || space2 == std::string::npos || buffer.find("\r\n") == std::string::npos) resp = E400(); //Проверка целостности запроса
      else{
        h_start = buffer.find("\r\n") + 2;
        colon = buffer.find(":", h_start); 
        end = buffer.find("\r\n", h_start);
        if(h_start + 2 > buffer.size()){
          resp = E400();
        }
        else{
          while(buffer.substr(h_start, 2) != "\r\n"){ //Заголовки
            colon = buffer.find(":", h_start);
            end = buffer.find("\r\n", h_start);
            if(colon == std::string::npos || end == std::string::npos){
              resp = E400();
              break;
            }
            req.headers[buffer.substr(h_start, colon - h_start)] = buffer.substr(colon + 2, end - colon - 2);
            h_start = end + 2;
            if(h_start + 2 > buffer.size()){
              resp = E400();
              break;
            }
          }
        }
      }
      if(resp != E400()){
        req.method = buffer.substr(0, space1); //Метод
        req.target = buffer.substr(space1 + 1, space2 - space1 - 1); //Цель
        req.version = buffer.substr(space2 + 1, buffer.find("\r\n") - space2 - 1); //Версия
        req.body = buffer.substr(h_start + 2); //Тело
        //Формирование ответа 
        if(req.method == "GET"){
          type = "text/plain";
          if(req.target == "/"){
            body = buffer;
            resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Content-Type: " + type + "\r\n"
            "\r\n" +
            body;
          }
          else if(req.target == "/IP"){
            const char* ntop_ret = inet_ntop(AF_INET, &cl_addr.sin_addr, cl_IP, INET_ADDRSTRLEN);
            if(ntop_ret == nullptr){
              std::cerr << "inet_ntop() error" << std::endl;
              strcpy(cl_IP, "unknown");
            }
            body.assign(cl_IP);
            body = body + ":" + std::to_string(ntohs(cl_addr.sin_port));
            resp =
            "HTTP/1.1 200 OK\r\n"
            "Content-Length: " + std::to_string(body.size()) + "\r\n"
            "Content-Type: " + type + "\r\n"
            "\r\n" +
            body; 
          }
          else resp = E404();
        }
        else{
          resp =
          "HTTP/1.1 405 Method Not Allowed\r\n"
          "Content-Type: text/plain\r\n"
          "Content-Length: 18\r\n"
          "\r\n"
          "Method Not Allowed";
        }
      }
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

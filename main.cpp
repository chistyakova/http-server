#include <iostream>
#include <sstream>
#include <cstring>
#include <fstream>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>

using std::cerr;

const char* ws = " \t\n\r\f\v";

// trim from end of string (right)
inline std::string& rtrim(std::string& s, const char* t = ws)
{
    s.erase(s.find_last_not_of(t) + 1);
    return s;
}

// trim from beginning of string (left)
inline std::string& ltrim(std::string& s, const char* t = ws)
{
    s.erase(0, s.find_first_not_of(t));
    return s;
}

// trim from both ends of string (right then left)
inline std::string& trim(std::string& s, const char* t = ws)
{
    return ltrim(rtrim(s, t), t);
}

int main()
{
    struct addrinfo* addr = NULL; // структура, хранящая информацию
    // об IP-адресе  слущающего сокета

    // Шаблон для инициализации структуры адреса
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));

    // AF_INET определяет, что используется сеть для работы с сокетом
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM; // Задаем потоковый тип сокета
    hints.ai_protocol = IPPROTO_TCP; // Используем протокол TCP
    // Сокет биндится на адрес, чтобы принимать входящие соединения
    hints.ai_flags = AI_PASSIVE;

    // Инициализируем структуру, хранящую адрес сокета - addr.
    // HTTP-сервер будет висеть на 8000-м порту локалхоста
    int result = getaddrinfo(NULL, "8000", &hints, &addr);

    // Если инициализация структуры адреса завершилась с ошибкой,
    // выведем сообщением об этом и завершим выполнение программы 
    if (result != 0) {
        cerr << "getaddrinfo failed: " << result << "\n";
        return 1;
    }

    // Создание сокета
    int listen_socket = socket(addr->ai_family, addr->ai_socktype,
        addr->ai_protocol);
    // Если создание сокета завершилось с ошибкой, выводим сообщение,
    // освобождаем память, выделенную под структуру addr
    if (listen_socket == -1) {
        cerr << "Error at socket\n";
        freeaddrinfo(addr);
        return 1;
    }
    
    // Делаем дескриптор сокета переиспользуемым
    int iSetOption = 1;
    setsockopt(listen_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&iSetOption, sizeof(iSetOption));
    
    // Привязываем сокет к IP-адресу
    result = bind(listen_socket, addr->ai_addr, (int)addr->ai_addrlen);

    // Если привязать адрес к сокету не удалось, то выводим сообщение
    // об ошибке, освобождаем память, выделенную под структуру addr.
    // и закрываем открытый сокет.
    
    if (result == -1) {
        cerr << "bind failed with error" << errno << "\n";
        
        freeaddrinfo(addr);
        close(listen_socket);
        return 1;
    }
    
    // Инициализируем слушающий сокет
    if (listen(listen_socket, SOMAXCONN) == -1) {
        cerr << "listen failed with error\n";
        close(listen_socket);
        return 1;
    }
    while(1) {
        
        // Принимаем входящие соединения
        int client_socket = accept(listen_socket, NULL, NULL);
        if (client_socket == -1) {
            cerr << "accept failed\n";
            close(listen_socket);
            return 1;
        }
        
        const int buffer_size = 1024;
        char buf[buffer_size];

        result = recv(client_socket, buf, buffer_size, 0);

        std::stringstream response; // сюда будет записываться ответ клиенту
        std::stringstream response_body; // тело ответа

        if (result == -1) {
            // ошибка получения данных
            cerr << "recv failed: " << result << "\n";
            close(client_socket);
        } else if (result == 0) {
            // соединение закрыто клиентом
            cerr << "connection closed...\n";
        } else if (result > 0) {
            // Мы знаем фактический размер полученных данных, поэтому ставим метку конца строки
            // В буфере запроса.
            buf[result] = '\0';
            std::string message = buf;
            auto pos1 = message.find("GET");
            auto pos2 = message.find("HTTP");
            auto request = message.substr(pos1+3, pos2-3);
            
            // Данные успешно получены
            // формируем тело ответа (HTML)
            response_body << "<title>Test C++ HTTP Server</title>\n"
            << "<h1>Test page</h1>\n"
            << "<p>This is body of the test page...</p>\n"
            << "<h2>Request headers</h2>\n"
            << "<pre>" << request << "</pre>\n"
            << "<em><small>Test C++ Http Server</small></em>\n";
            
            std::ifstream istrm("."+trim(request), std::ios::binary);
            std::cerr << "1" << request << "1\n";
            if(istrm.is_open()) {
                std::string s;
                std::string result;
                while (istrm >> s) {
                    result+=s;
                }
                // Формируем весь ответ вместе с заголовками
                    response << "HTTP/1.1 200 OK\r\n"
                    << "Version: HTTP/1.1\r\n"
                    << "Content-Type: text/html; charset=utf-8\r\n"
                    << "Content-Length: " << result.length()
                    << "\r\n\r\n"
                    << result;
                    std::cerr <<  result << "\n";
            } else {
                response << "HTTP/1.0 404 NOT FOUND\r\n"
                << "Content-Type: text/html\r\n\r\n";
            }

            // Отправляем ответ клиенту с помощью функции send
            result = send(client_socket, response.str().c_str(),
                response.str().length(), 0);

            if (result == -1) {
                // произошла ошибка при отправле данных
                cerr << "send failed\n";
            }
            // Закрываем соединение к клиентом
            close(client_socket);
        }
    }
    
    // Убираем за собой
    close(listen_socket);
    freeaddrinfo(addr);
    return 0;
}

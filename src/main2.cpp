#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <cstring>
#include <cstdio>
#include <iostream>
#include <fstream>

#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

std::map<std::string, std::string> url_mapping = {
    {"/", "../templates/index.html"},
    {"/about", "../templates/about.html"},
    {"/resume.pdf", "../resume_PDF/resume.pdf"},
    {"/profile_pic.jpeg", "../media/profile_pic/profile_pic.jpeg"},
    {"/energy_harvesting.jpg", "../media/energyHarvesting/energy_harvesting.jpg"},
    {"/energyHarvesting2.png", "../media/energyHarvesting/energyHarvesting2.png"},
    {"/poster.png", "../media/energyHarvesting/poster.png"},
    {"/antenna_IRL.png", "../media/birchAntenna/antenna_IRL.png"},
    {"/PCB_IRL.jpg", "../media/electricalEnergySystem/PCB_IRL.jpg"},
    {"/greenhouse.png", "../media/greenHouse/greenhouse.png"},
    {"/rc_car.png", "../media/rpCar/rc_car.png"},
    {"/Sorted_cubes.png", "../media/ROS2_robotic_arm/Sorted_cubes.png"},
    {"/weatherApp.png", "../media/pythonWeatherApp/weatherApp.png"},
    {"/RC_car.gif", "../media/rpCar/RC_car.gif"},
    {"/video_dem.gif", "../media/ROS2_robotic_arm/video_dem.gif"},
    {"/termo.gif", "../media/energyHarvesting/termo.gif"},
    {"/cpp.png", "../media/skills/cpp.png"},
    {"/c.png", "../media/skills/c.png"},
    {"/linux.png", "../media/skills/linux.png"},
    {"/python.png", "../media/skills/python.png"},
    {"/git.png", "../media/skills/git.png"}
};

std::pair<std::string, std::string> parse_request(const std::string& path_request){
    std::istringstream stream(path_request);
    
    std::string line;
    std::getline(stream,line);
    std::istringstream new_line(line);

    std::vector<std::string> tokens;
    std::string token;

    while(new_line >> token){
        tokens.push_back(token);
    }

    std::string method;
    if(tokens.size() > 0){
        method = tokens[0];
    }else{
        method = "";
    }

    std::string path;
    if(tokens.size() > 1){
        path = tokens[1];
    }else{
        path = "";
    }

    return std::make_pair(method, path);
}

void handlePostRequest(int client_socket, int content_length, const std::string& post_data){
    std::istringstream stream(post_data);
    std::string line;
    std::map<std::string, std::string> parsed_data;

    while(std::getline(stream, line, '&')){
        size_t pos = line.find('=');
        if(pos != std::string::npos){
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            parsed_data[key] = value;
        }
    }

    std::string name = parsed_data["name"];

    // for debugging;
    std::cout << "Received name: " << name << std::endl;
    
    // Construct single clean HTTP response
    std::string response_data = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
    response_data += "<h1>Hello, " + name + "!</h1>";
    
    // Fixed path to match url_mapping style
    std::ifstream file("../templates/index.html");
    if(file.is_open()){
        // Removed the extra duplicate HTTP/1.1 header injection here
        response_data += std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
        file.close();
    }

    send(client_socket, response_data.c_str(), response_data.size(), 0);
    close(client_socket);
}

void handleGetRequest(int client_socket, std::string& request_path, int content_length, std::string& post_data){
    std::string response;
    // we use post request
    if(url_mapping.find(request_path) != url_mapping.end() && content_length != 0){
        handlePostRequest(client_socket, content_length, post_data);
    }else{
        if(url_mapping.find(request_path) != url_mapping.end()){
            std::string file_path = url_mapping[request_path];
            std::ifstream file(file_path, std::ios::binary); //Read mode binary
            if(file.is_open()){
                std::string content_type = "text/html";
                
                if(request_path.find(".pdf") != std::string::npos){
                    content_type = "application/pdf";
                }else if(request_path.find(".png") != std::string::npos){
                    content_type = "image/png";
                }else if(request_path.find(".jpg") != std::string::npos){
                    content_type = "image/jpeg";
                }else if(request_path.find(".gif") != std::string::npos){
                    content_type = "image/gif";
                }
            std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
            file.close();
            response = "HTTP/1.1 200 OK\r\nContent-Type: " + content_type + "\r\nContent-Length: "  + std::to_string(file_content.size()) + "\r\n\r\n" + file_content;

            }else{
                response = "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n<h1>404 not found</h1>";
            }
            send(client_socket, response.c_str(), response.size(), 0);
            close(client_socket);
        }
    }
}

void runServer(){
    int port = 8081;
    const char* ip_host = "127.0.0.1";

    int socket_server = socket(AF_INET, SOCK_STREAM, 0);
    if(socket_server == -1){
        std::cerr << "Error in creating socket!" << std::endl;
    }

    sockaddr_in server_address{};
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(ip_host);

    if(bind(socket_server, (struct sockaddr*)&server_address, sizeof(server_address)) != 0){
        std::cerr << "Error in binding socket!" << std::endl;
        close(socket_server);
    }

    if(listen(socket_server, 1) == -1){
        std::cerr << "Failed to listen on socket" << std::endl;
        close(socket_server);
    }

    std::cout << "Server listening on : " << ip_host << ":" << port << std::endl; 

    try{
        while(true){
            sockaddr_in client_address{};
            socklen_t client_address_size = sizeof(client_address);

            int client_socket = accept(socket_server, reinterpret_cast<sockaddr*>(&client_address), &client_address_size);

            if(client_socket == -1){
                std::cerr << "Error in accepting connection" << std::endl;
                continue;
            }

            std::cout << "Accepted connection from " << inet_ntoa(client_address.sin_addr) << std::endl;
            char buffer[1024];
            int bytes_recieved = recv(client_socket, buffer, sizeof(buffer), 0);

            if(bytes_recieved == -1){
                std::cerr << "Error in receiving bytes" << std::endl;
                close(client_socket);
                continue;
            }

            std::string request_data(buffer, bytes_recieved);
            auto [method, path] = parse_request(request_data);
            std::cout << "Received " << method << " for " << path << std::endl;

            int content_length = 0;
            std::string post_data;
            if(method == "POST"){
                size_t pos = request_data.find("\r\n\r\n");
                if(pos != std::string::npos){
                    std::string headers = request_data.substr(0, pos);
                    std::istringstream headers_stream(headers);
                    std::string header;
                    while(std::getline(headers_stream, header, '\r')){
                        // Fixed: Checked for capital 'L' in Content-Length
                        if(header.find("Content-Length: ") != std::string::npos || header.find("Content-length: ") != std::string::npos){
                            size_t colon = header.find(':');
                            content_length = std::stoi(header.substr(colon + 2));
                        }
                    }
                    // Fixed: pos + 4 skips "\r\n\r\n" correctly instead of pos + 1
                    post_data = request_data.substr(pos + 4);
                }
            }
            handleGetRequest(client_socket, path, content_length, post_data);
        }
    }catch(const std::exception& e){
        std::cerr << "Exception: " << e.what() << std::endl;
    }catch(...){
        std::cerr << "Unknown exception" << std::endl; 
    }
}

int main(){
    runServer();
}
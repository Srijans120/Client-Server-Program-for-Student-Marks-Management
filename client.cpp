#include <iostream>
#include <sstream>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>  // Include this header for memset()

#define PORT 8080

void displayMenu(bool isInstructor) {
    if (isInstructor) {
        std::cout << "1. View all students' marks\n";
        std::cout << "2. View class average\n";
        std::cout << "3. View number of students failed in each subject\n";
        std::cout << "4. View best and worst performing students\n";
        std::cout << "5. Update a student's marks\n";
    } else {
        std::cout << "1. View your marks\n";
        std::cout << "2. View your aggregate percentage\n";
        std::cout << "3. View subjects with maximum and minimum marks\n";
    }
    std::cout << "0. Exit\n";  // Add Exit option
    std::cout << "Enter your choice: ";
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cout << "Socket creation error\n";
        return -1;
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        std::cout << "Invalid address/ Address not supported\n";
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cout << "Connection failed\n";
        return -1;
    }
    
    // Authentication
    std::string username, password;
    std::cout << "Enter username: ";
    std::cin >> username;
    std::cout << "Enter password: ";
    std::cin >> password;
    
    std::string login_info = username + " " + password;
    send(sock, login_info.c_str(), login_info.size(), 0);
    memset(buffer, 0, sizeof(buffer));  // Clear the buffer before reading
    read(sock, buffer, 1024);
    std::cout << buffer << std::endl;
    
    if (std::string(buffer).find("Failed") != std::string::npos) {
        close(sock);
        return 0;
    }
    
    bool isInstructor = (username == "instructor");
    
    // Main Menu
    while (true) {
        displayMenu(isInstructor);
        int choice;
        std::cin >> choice;
        
        if (choice == 0) {  // Exit option
            std::string exit_msg = "exit";
            send(sock, exit_msg.c_str(), exit_msg.size(), 0);
            break;
        }

        std::ostringstream oss;
        oss << choice;
        
        if (isInstructor && choice == 5) {
            // Update marks
            std::string student;
            int subject, new_marks;
            std::cout << "Enter student name: ";
            std::cin >> student;
            std::cout << "Enter subject number (1-5): ";
            std::cin >> subject;
            std::cout << "Enter new marks: ";
            std::cin >> new_marks;
            oss << " " << student << " " << subject << " " << new_marks;
        }
        std::string request = oss.str();
        send(sock, oss.str().c_str(), oss.str().size(), 0);
        memset(buffer, 0, sizeof(buffer));  // Clear the buffer before reading
        read(sock, buffer, 1024);
        std::cout << buffer << std::endl;
    }
    
    close(sock);
    return 0;
}

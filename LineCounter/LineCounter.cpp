// LineCounter.cpp : This file contains the 'main' function. Program execution begins and ends there.
//
#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <deque>
#include <mutex>

namespace fs = std::filesystem;

// CounterWorker
// performs a thread safe calculation of the number of lines in files
// file_names -> reference to the std::vector structure contains names of files to process
// vecLock -> std::mutex object to controll access to the fine_names
// counter -> atomic int, to store the result
void counterWorker(std::deque<std::string>& file_names, std::mutex& vecLock, std::atomic<uint32_t>& counter)
{
    std::string file_name;
    std::ifstream ifs;

    while (!file_names.empty())
    {
        vecLock.lock();
        file_name = file_names.back();
        file_names.pop_back();
        vecLock.unlock();

        ifs.open(file_name);
        while (ifs.ignore(INT64_MAX, '\n'))
        {
            counter++;
        }
        //we skip eof check, as we don't want to proceed the errors.
        ifs.close();
    }
}


int main(int argc, char* argv[])
{
    //little args parcer
    if (argc != 2)
    {
        std::cout << "usage: linecounter dir\n";
        return 1;
    }
    std::filesystem::path dir(argv[1]);
    if (!fs::exists(dir))
    {
        std::cout << "specified path not found!\n";
        return 1;
    }
    if (!fs::is_directory(dir))
    {
        std::cout << "usage: linecounter dir\n";
        return 1;
    }

    //this code is responcive to creating a file_name list
    int filesCount = 0;
    //almost the same as std::vector but looks like it a bit fatser
    std::deque<std::string> file_names;
    //for c++11 and lover boost lib can be used
    for (const fs::directory_entry& entry :
        fs::recursive_directory_iterator(dir))
    {
        if (entry.is_regular_file())
        {
            file_names.push_back(fs::absolute(entry.path()).string());
            filesCount++;
        }
    }

    //calculate the target threads count
    uint32_t threadsNum = std::thread::hardware_concurrency();
    threadsNum = threadsNum < filesCount ? threadsNum : filesCount;

    std::cout << "running counter routine in " << threadsNum << " threads\n";
    std::atomic<uint32_t> n = 0;
    std::mutex vecLock;
    std::vector<std::thread> threads;
    for (int i = 0; i < threadsNum; i++)
    {
        threads.push_back(std::thread(counterWorker, std::ref(file_names), std::ref(vecLock), std::ref(n)));
    }

    std::cout << "synchronizing all threads...\n";
    for (std::thread& th : threads)
    {
        th.join();
    }

    std::cout << "total " << filesCount << " files proccessed, the lines count is " << n << "\n";
}
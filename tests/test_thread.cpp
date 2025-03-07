#include <stdio.h>
#include <thread>
#include <vector>
#include <atomic>
#include <iostream>


std::atomic<int> counter(0);

void increment ()
{

    for (size_t i = 0; i < 10000; i++)
    {
        counter++;
    }
    
}

int main(int argc, char const *argv[])
{
    std::vector<std::thread> vecs;
    for (int i = 0; i < 10; i++)
    {
        vecs.emplace_back(increment);
    }
    
    for (auto &i : vecs)
    {
        i.join();
    }
    
    std::cout << counter << std::endl;

    return 0;
}

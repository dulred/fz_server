#include <stdio.h>
#include <thread>
#include <chrono>
//  linux need to link pthread lib ,otherwise fail
int main(int argc, char const *argv[])
{
    std::thread t ([](){
        printf("are you ok?");
        std::this_thread::sleep_for(std::chrono::seconds(2));
    });

    // t.join();
    // t.detach();
    if (t.joinable())
    {
        t.join();
    }
    
    printf("done");
    return 0;
}

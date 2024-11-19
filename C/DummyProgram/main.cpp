#include <thread>
#include <chrono>

int main(int argc, char *argv[])
{
    int a = 5;
    int b = 12;
    int c = a + b;
    std::this_thread::sleep_for(std::chrono::seconds(200));

    return 0;
}

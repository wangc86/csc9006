#include <iostream>
#include <chrono>
#include <cmath>

int main (void)
{
    std::chrono::system_clock::time_point startTime = std::chrono::system_clock::now();
    for (int i = 1; i < 100000; i++)
    {
        //for (int j = 1; j < 113; j++) // for powersave governor
        for (int j = 1; j < 57; j++) // for performance governor
        {
            sqrt(i*j*i);
        }
    }
    std::chrono::system_clock::time_point endTime = std::chrono::system_clock::now();
    const int delta = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    //std::cout << "response time of our workload = " << delta << " ms" << std::endl;
    std::cout << delta << std::endl;
    return 0;
}

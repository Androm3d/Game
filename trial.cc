#include <iostream>

using namespace std;

int main()
{
    int k = 2;
    bool next = true;
    while (next) {  //check straight before corners
        cout << k << endl;
        next = k != 7;
        k += 2;
        if (k != 7) k %= 7;
    }
}
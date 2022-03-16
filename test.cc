#include <iostream>
#include <vector>
#include <queue>
#include <map>
#include <fstream>
#include <limits.h>

#include "thread.h"

using namespace std;

void foo() {
    cout << 1 << endl;
}

int main(int argc, char* argv[]) {
    thread_libinit(foo, nullptr);
    return 0;
}

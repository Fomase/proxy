#include <cstdint>
#include <iostream>

using namespace std;

// упростите эту экспоненциальную функцию,
// реализовав линейный алгоритм
int64_t T(int i) {
    if (i == 1 || i ==0) {
        return 0;
    }
    int t1 = 0;
    int t2 = 0;
    int t3 = 1;
    int hold;
    
    for (int j = 2; j < i; ++j) {
        hold = t3;
        t3 += t2 + t1;
        t1 = t2;
        t2 = hold;
    }
    return t3;
}

int main() {
    int i;

    while (true) {
        cout << "Enter index: "s;
        if (!(cin >> i)) {
            break;
        }

        cout << "Ti = "s << T(i) << endl;
    }
}

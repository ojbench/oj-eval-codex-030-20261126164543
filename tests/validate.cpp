#include "../src.hpp"
#include <chrono>
#include <iostream>

int main() {
    auto start = std::chrono::steady_clock::now();
    int correct = 0;
    int confusion[10][10] = {};
    for (int k = 0; k < 100; ++k) {
        IMAGE_T img = digit_recognition::reference_image(k);
        int prediction = digit_recognition::classify(img, k);
        correct += prediction == k/10;
        ++confusion[k/10][prediction];
    }
    std::cout << "Leave-one-out: " << correct << "/100\n";
    for (auto &row : confusion) {
        for (int n : row) std::cout << n << ' ';
        std::cout << '\n';
    }
    std::cout << "Seconds: " << std::chrono::duration<double>(
        std::chrono::steady_clock::now()-start).count() << '\n';
}

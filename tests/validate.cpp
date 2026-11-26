#include "../src.hpp"
#include <chrono>
#include <iostream>

int main() {
    auto start = std::chrono::steady_clock::now();
    int correct = 0;
    int confusion[10][10] = {};
    for (int k = 0; k < 100; ++k) {
        IMAGE_T img(28, std::vector<double>(28));
        auto hex = [](char c) { return c <= '9' ? c-'0' : c-'a'+10; };
        for (int p = 0; p < 784; ++p)
            img[p/28][p%28] = (16*hex(digit_recognition::samples[k][2*p])+
                hex(digit_recognition::samples[k][2*p+1]))/255.0;
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

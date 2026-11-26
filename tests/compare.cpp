#include "../src.hpp"
#include <iostream>

int main() {
    using namespace digit_recognition;
    const auto &refs = references();
    float deform[100][100] = {}, gradient[100][100] = {}, topology[100][100] = {};
    for (int a = 0; a < 100; ++a)
        for (int b = 0; b < a; ++b) {
            deform[a][b] = deform[b][a] =
                0.2f*(distance(refs[a].plain,refs[b].plain)+distance(refs[b].plain,refs[a].plain))+
                0.3f*(distance(refs[a].straight,refs[b].straight)+distance(refs[b].straight,refs[a].straight));
            gradient[a][b] = gradient[b][a] =
                0.4f*hog_distance(refs[a].plain,refs[b].plain)+
                0.6f*hog_distance(refs[a].straight,refs[b].straight);
            topology[a][b] = topology[b][a] =
                0.4f*hole_distance(refs[a].plain,refs[b].plain)+
                0.6f*hole_distance(refs[a].straight,refs[b].straight);
        }
    for (float weight : {0.f, 4.f, 8.f, 16.f, 32.f, 64.f, 128.f}) {
        int correct = 0;
        for (int a = 0; a < 100; ++a) {
            float best = 1e9f;
            int answer = -1;
            for (int b = 0; b < 100; ++b) {
                if (a == b) continue;
                float d = deform[a][b]+32*gradient[a][b]+weight*topology[a][b];
                if (d < best) { best = d; answer = b/10; }
            }
            correct += answer == a/10;
        }
        std::cout << weight << ' ' << correct << '\n';
    }
}

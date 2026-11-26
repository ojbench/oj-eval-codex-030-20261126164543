using Image = std::vector<std::vector<double>>;
constexpr int side = 24;
constexpr int area = side * side;
struct Feature {
    std::array<float, area> ink{}, dx{}, dy{};
};

inline double sample(const Image &img, double y, double x) {
    int iy = static_cast<int>(std::floor(y));
    int ix = static_cast<int>(std::floor(x));
    double result = 0;
    for (int j = 0; j < 2; ++j)
        for (int i = 0; i < 2; ++i)
            if (iy+j >= 0 && iy+j < static_cast<int>(img.size()) &&
                ix+i >= 0 && ix+i < static_cast<int>(img[iy+j].size()))
                result += img[iy+j][ix+i] * (j ? y-iy : 1-y+iy) *
                          (i ? x-ix : 1-x+ix);
    return result;
}

inline Feature extract(const Image &img, bool deskew) {
    double mass = 0, cx = 0, cy = 0;
    for (int y = 0; y < static_cast<int>(img.size()); ++y)
        for (int x = 0; x < static_cast<int>(img[y].size()); ++x) {
            double p = img[y][x];
            mass += p; cx += x*p; cy += y*p;
        }
    Feature f;
    if (mass < 1e-8) return f;
    cx /= mass; cy /= mass;
    double xy = 0, yy = 0;
    for (int y = 0; y < static_cast<int>(img.size()); ++y)
        for (int x = 0; x < static_cast<int>(img[y].size()); ++x) {
            xy += img[y][x]*(x-cx)*(y-cy);
            yy += img[y][x]*(y-cy)*(y-cy);
        }
    double shear = deskew ? std::clamp(xy / (yy+1e-8), -0.8, 0.8) : 0;
    double left = 100, right = -100, top = 100, bottom = -100;
    for (int y = 0; y < static_cast<int>(img.size()); ++y)
        for (int x = 0; x < static_cast<int>(img[y].size()); ++x)
            if (img[y][x] > 0.15) {
                double xx = x - shear*(y-cy);
                left = std::min(left, xx); right = std::max(right, xx);
                top = std::min(top, double(y)); bottom = std::max(bottom, double(y));
            }
    if (right < left) return f;
    double scale = (bottom-top+1)/20.0;
    double xscale = std::max(right-left+1, (bottom-top+1)*0.5)/16.0;
    double mx = (left+right)/2, my = (top+bottom)/2;
    for (int y = 0; y < side; ++y)
        for (int x = 0; x < side; ++x) {
            double sy = my + (y-11.5)*scale;
            double sx = mx + (x-11.5)*xscale + shear*(sy-cy);
            f.ink[y*side+x] = static_cast<float>(sample(img, sy, sx));
        }
    for (int y = 1; y < side-1; ++y)
        for (int x = 1; x < side-1; ++x) {
            int p = y*side+x;
            f.dx[p] = (f.ink[p+1]-f.ink[p-1])*0.5f;
            f.dy[p] = (f.ink[p+side]-f.ink[p-side])*0.5f;
        }
    return f;
}

struct Reference {
    Feature plain, straight;
};
inline Image reference_image(int k) {
    Image img(28, std::vector<double>(28));
    auto hex = [](char c) { return c <= '9' ? c-'0' : c-'a'+10; };
    int offset = 0;
    auto byte = [&]() {
        int value = 16*hex(samples[k][offset])+hex(samples[k][offset+1]);
        offset += 2;
        return value;
    };
    for (int p = 0; p < 784;) {
        int value = byte();
        if (value == 0) p += byte();
        else { img[p/28][p%28] = value/255.0; ++p; }
    }
    return img;
}
inline const std::array<Reference,100>& references() {
    static const auto refs = [] {
        std::array<Reference,100> result;
        for (int k = 0; k < 100; ++k) {
            Image img = reference_image(k);
            result[k] = {extract(img, false), extract(img, true)};
        }
        return result;
    }();
    return refs;
}

inline float distance(const Feature &a, const Feature &b) {
    float sum = 0;
    // Locally deform a stroke by at most one pixel, comparing its intensity
    // and two derivatives together so that unrelated edges cannot match.
    for (int y = 1; y < side-1; ++y)
        for (int x = 1; x < side-1; ++x) {
            int p = y*side+x;
            float best = 1e9f;
            for (int dy = -1; dy <= 1; ++dy)
                for (int dx = -1; dx <= 1; ++dx) {
                    int q = p + dy*side + dx;
                    float v = a.ink[p]-b.ink[q];
                    float gx = a.dx[p]-b.dx[q], gy = a.dy[p]-b.dy[q];
                    float d = v*v + gx*gx + gy*gy;
                    best = std::min(best, d);
                }
            sum += best;
        }
    return sum;
}

inline int classify(const Image &img, int excluded = -1) {
    Feature a = extract(img, false), b = extract(img, true);
    const auto &refs = references();
    float best = 1e9f;
    int answer = 0;
    for (int k = 0; k < 100; ++k) {
        if (k == excluded) continue;
        float d = 0.2f*(distance(a, refs[k].plain)+distance(refs[k].plain, a)) +
                  0.3f*(distance(b, refs[k].straight)+distance(refs[k].straight, b));
        if (d < best) { best = d; answer = k/10; }
    }
    return answer;
}

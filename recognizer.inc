using Image = std::vector<std::vector<double>>;
constexpr int side = 24;
constexpr int area = side * side;
struct Feature {
    std::array<float, area> ink{}, dx{}, dy{};
    std::array<float, 288> hog{};
    std::array<float, 3> holes{};
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
    float cells[4][4][8] = {};
    for (int y = 1; y < side-1; ++y)
        for (int x = 1; x < side-1; ++x) {
            int p = y*side+x;
            float gx = f.dx[p], gy = f.dy[p];
            float magnitude = std::sqrt(gx*gx+gy*gy);
            float angle = std::atan2(gy, gx)*1.273239545f + 8;
            int bin = static_cast<int>(angle);
            float fraction = angle-bin;
            float cellx = (x+0.5f)/6-0.5f, celly = (y+0.5f)/6-0.5f;
            int ix = static_cast<int>(std::floor(cellx));
            int iy = static_cast<int>(std::floor(celly));
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx) {
                    int cx = ix+dx, cy = iy+dy;
                    if (cx < 0 || cx >= 4 || cy < 0 || cy >= 4) continue;
                    float w = magnitude*(dx ? cellx-ix : 1-cellx+ix)*
                              (dy ? celly-iy : 1-celly+iy);
                    cells[cy][cx][bin%8] += w*(1-fraction);
                    cells[cy][cx][(bin+1)%8] += w*fraction;
                }
        }
    int h = 0;
    for (int y = 0; y < 3; ++y)
        for (int x = 0; x < 3; ++x) {
            float norm = 0.01f;
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    for (float v : cells[y+dy][x+dx]) norm += v*v;
            norm = std::sqrt(norm);
            for (int dy = 0; dy < 2; ++dy)
                for (int dx = 0; dx < 2; ++dx)
                    for (float v : cells[y+dy][x+dx]) f.hog[h++] = v/norm;
        }
    // Average enclosed-background measurements across thresholds to avoid
    // making a hard topological decision from a single antialiased pixel.
    for (float threshold : {0.25f, 0.5f, 0.75f}) {
        bool seen[area] = {};
        for (int p = 0; p < area; ++p) {
            if (seen[p] || f.ink[p] >= threshold) continue;
            int queue[area], begin = 0, end = 1;
            queue[0] = p; seen[p] = true;
            bool border = false;
            float center = 0;
            while (begin < end) {
                int q = queue[begin++], y = q/side, x = q%side;
                center += y;
                if (x == 0 || y == 0 || x == side-1 || y == side-1) border = true;
                for (int offset : {-side, -1, 1, side}) {
                    int r = q+offset;
                    if (r < 0 || r >= area || (offset == -1 && x == 0) ||
                        (offset == 1 && x == side-1)) continue;
                    if (!seen[r] && f.ink[r] < threshold) {
                        seen[r] = true; queue[end++] = r;
                    }
                }
            }
            if (!border && end >= 3) {
                float strength = std::min(1.0f, end/12.0f)/3.0f;
                float position = center/end/(side-1);
                f.holes[0] += strength;
                f.holes[1] += strength*(1-position);
                f.holes[2] += strength*position;
            }
        }
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

inline float hog_distance(const Feature &a, const Feature &b) {
    float d = 0;
    for (int i = 0; i < 288; ++i) {
        float v = a.hog[i]-b.hog[i];
        d += v*v;
    }
    return d;
}

inline float hole_distance(const Feature &a, const Feature &b) {
    float d = 0;
    for (int i = 0; i < 3; ++i) {
        float v = a.holes[i]-b.holes[i];
        d += v*v;
    }
    return d;
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
        d += 32.0f*(0.4f*hog_distance(a, refs[k].plain)+
                    0.6f*hog_distance(b, refs[k].straight));
        d += 32.0f*(0.4f*hole_distance(a, refs[k].plain)+
                    0.6f*hole_distance(b, refs[k].straight));
        if (d < best) { best = d; answer = k/10; }
    }
    return answer;
}


#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <iostream>
#include <fstream>
#include <string>
#include <cmath>
#include <queue>
#include <vector>


using namespace std;

// ---- Utility ----
int clip(int v, int lo = 0, int hi = 255);
unsigned char* toGrayscale(const unsigned char* rgba, int width, int height);

// ---- Canny Edge Detection ----
void gaussianBlur(const unsigned char* src, unsigned char* dst, int w, int h);
void sobel(const unsigned char* src, int* Gx, int* Gy, int w, int h);
void nonMaxSuppression(const unsigned char* mag, const int* Gx, const int* Gy, unsigned char* nms, int w, int h);
void doubleThreshold(const unsigned char* nms, unsigned char* dt, int w, int h, unsigned char lowThresh, unsigned char highThresh);
void hysteresis(unsigned char* dt, int w, int h);

unsigned char* cannyEdgeDetection(const unsigned char* gray, int width, int height, unsigned char lowThresh, unsigned char highThresh);

// ---- Halftone ----
unsigned char* makeHalftone(const unsigned char* gray, int width, int height, int& outWidth, int& outHeight);

// ---- Floyd–Steinberg ----
unsigned char* floydSteinberg16(const unsigned char* gray, int width, int height);







int main() {
    string filepath = "res/textures/Lenna.png";
    int width, height, comps;
    int req_comps = 4;

    // Load image as RGBA
    unsigned char* img = stbi_load(filepath.c_str(), &width, &height, &comps, req_comps);
    if (!img) {
        cout << "Error: failed to load image" << endl;
        return 1;
    }

    // ---------- Grayscale ----------
    int grayWidth = width;
    int grayHeight = height;
    unsigned char* gray = toGrayscale(img, width, height);

    stbi_write_png("res/textures/Grayscale.png", width, height, 1, gray, width);


    ofstream out("Grayscale.txt");
    if (out.is_open()) {
        for (int i = 0; i < grayWidth * grayHeight; i++) {
            int level16 = gray[i] / 16;   // 0..15
            out << level16;
            if (i != grayWidth * grayHeight - 1)
                out << ",";
        }
        out.close();
    }

    
    // ---------- Canny ----------

    unsigned char* edges = cannyEdgeDetection(gray, grayWidth, grayHeight, 30, 80);

    stbi_write_png("res/textures/Canny.png", grayWidth, grayHeight, 1, edges, grayWidth);

    ofstream cannyOut("Canny.txt");
    if (cannyOut.is_open()) {
        for (int i = 0; i < grayWidth * grayHeight; i++) {
            int bit = (edges[i] > 0) ? 1 : 0;
            cannyOut << bit;
            if (i != grayWidth * grayHeight - 1)
                cannyOut << ",";
        }
        cannyOut.close();
    }



    // ---------- Halftone ----------
    int halfW, halfH;
    unsigned char* halftone = makeHalftone(gray, grayWidth, grayHeight, halfW, halfH);

    stbi_write_png("res/textures/Halftone.png", halfW, halfH, 1, halftone, halfW);

    ofstream hout("Halftone.txt");
    if (hout.is_open()) {
        for (int i = 0; i < halfW * halfH; i++) {
            int v = (halftone[i] == 0 ? 0 : 1);
            hout << v;
            if (i != halfW * halfH - 1)
                hout << ",";
        }
        hout.close();
    }
    

    
    // ---------- Floyd–Steinberg ----------
    unsigned char* fs = floydSteinberg16(gray, grayWidth, grayHeight);

    stbi_write_png("res/textures/FloyedSteinberg.png", grayWidth, grayHeight, 1, fs, grayWidth);

    ofstream fsOut("FloyedSteinberg.txt");
    if (fsOut.is_open()) {
        for (int i = 0; i < grayWidth * grayHeight; i++) {
            int val16 = (int)std::round(fs[i] / 255.0f * 15.0f); 
            fsOut << val16;
            if (i != grayWidth * grayHeight - 1)
                fsOut << ",";
        }
        fsOut.close();
    }



    // cleanup
    delete[] gray;
    delete[] fs;
    delete[] edges;
    stbi_image_free(img);
    free(halftone);

    cout << "Done. Created Grayscale, FloydSteinberg, Halftone, Canny PNG+TXT." << endl;
    return 0;
}



int clip(int v, int lo, int hi) {
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

unsigned char* toGrayscale(const unsigned char* rgba, int width, int height)
{
    int size = width * height;
    unsigned char* gray = new unsigned char[size];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {

            int idx = (y * width + x) * 4;  // RGBA index

            unsigned char R = rgba[idx + 0];
            unsigned char G = rgba[idx + 1];
            unsigned char B = rgba[idx + 2];

            // average method
            gray[y * width + x] = (R + G + B) / 3;
        }
    }

    return gray;
}


void gaussianBlur(const unsigned char* src, unsigned char* dst, int w, int h) {
    //5x5 Gaussian kernel
    int K[5][5] = {
        { 2,  4,  5,  4, 2 },
        { 4,  9, 12,  9, 4 },
        { 5, 12, 15, 12, 5 },
        { 4,  9, 12,  9, 4 },
        { 2,  4,  5,  4, 2 }
    };
    int Ksum = 159;

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int acc = 0;
            for (int ky = -2; ky <= 2; ky++) {
                for (int kx = -2; kx <= 2; kx++) {
                    int ix = x + kx;
                    int iy = y + ky;
                    if (ix < 0 || ix >= w || iy < 0 || iy >= h) continue;
                    int k = K[ky + 2][kx + 2];
                    acc += k * src[iy * w + ix];
                }
            }
            dst[y * w + x] = (unsigned char)clip(acc / Ksum);
        }
    }
}
void sobel(const unsigned char* src, int* Gx, int* Gy,int w, int h){
    int Sx[3][3] = {
        { -1, 0, 1 },
        { -2, 0, 2 },
        { -1, 0, 1 }
    };

    int Sy[3][3] = {
        { -1, -2, -1 },
        {  0,  0,  0 },
        {  1,  2,  1 }
    };

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {

            int gx = 0;
            int gy = 0;

            // 3x3 convolution around (x,y)
            for (int ky = -1; ky <= 1; ky++) {
                for (int kx = -1; kx <= 1; kx++) {

                    int ix = x + kx;
                    int iy = y + ky;

                    if (ix < 0 || ix >= w || iy < 0 || iy >= h) continue;

                    unsigned char p = src[iy * w + ix];

                    gx += p * Sx[ky + 1][kx + 1];
                    gy += p * Sy[ky + 1][kx + 1];
                }
            }

            Gx[y * w + x] = gx;
            Gy[y * w + x] = gy;
        }
    }
}
void nonMaxSuppression(const unsigned char* mag, const int* Gx, const int* Gy, unsigned char* nms, int w, int h){
    // initialize to 0
    for (int i = 0; i < w * h; i++)
        nms[i] = 0;

    for (int y = 1; y < h - 1; y++) {
        for (int x = 1; x < w - 1; x++) {

            int idx = y * w + x;
            float gx = (float)Gx[idx];
            float gy = (float)Gy[idx];

            float angle = atan2(gy, gx) * 180.0f / 3.14159265f;
            if (angle < 0)
                angle += 180.0f;

            unsigned char m = mag[idx];
            unsigned char n1 = 0, n2 = 0;

            // Quantize direction to 0,45,90,135 degrees
            if ((angle >= 0   && angle < 22.5) ||
                (angle >= 157.5 && angle <= 180.0)) {
                // horizontal edge – compare left/right
                n1 = mag[idx - 1];
                n2 = mag[idx + 1];
            }
            else if (angle >= 22.5 && angle < 67.5) {
                // 45 degrees – compare diag (up-right / down-left)
                n1 = mag[(y - 1) * w + (x + 1)];
                n2 = mag[(y + 1) * w + (x - 1)];
            }
            else if (angle >= 67.5 && angle < 112.5) {
                // vertical edge – compare up/down
                n1 = mag[(y - 1) * w + x];
                n2 = mag[(y + 1) * w + x];
            }
            else {
                // 135 degrees – compare diag (up-left / down-right)
                n1 = mag[(y - 1) * w + (x - 1)];
                n2 = mag[(y + 1) * w + (x + 1)];
            }

            // keep only if pixel is local maximum
            if (m >= n1 && m >= n2)
                nms[idx] = m;
            else
                nms[idx] = 0;
        }
    }
}void doubleThreshold(const unsigned char* nms, unsigned char* dt, int w, int h, unsigned char lowThresh, unsigned char highThresh){
    for (int i = 0; i < w * h; i++) {
        unsigned char v = nms[i];
        if (v >= highThresh)
            dt[i] = 255;      // strong edge
        else if (v >= lowThresh)
            dt[i] = 128;      // weak edge
        else
            dt[i] = 0;        // non-edge
    }
}
void hysteresis(unsigned char* dt, int w, int h){
    queue<int> q;

    // start BFS from all strong edges
    for (int i = 0; i < w * h; i++) {
        if (dt[i] == 255)
            q.push(i);
    }

    // promote connected weak edges to strong
    while (!q.empty()) {
        int idx = q.front();
        q.pop();

        int x = idx % w;
        int y = idx / w;

        for (int dy = -1; dy <= 1; dy++) {
            for (int dx = -1; dx <= 1; dx++) {
                if (dx == 0 && dy == 0) continue;

                int nx = x + dx;
                int ny = y + dy;
                if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;

                int nidx = ny * w + nx;
                if (dt[nidx] == 128) {    // weak edge
                    dt[nidx] = 255;       // promote to strong
                    q.push(nidx);
                }
            }
        }
    }

    // remove remaining weak edges
    for (int i = 0; i < w * h; i++) {
        if (dt[i] == 128)
            dt[i] = 0;
    }
}
unsigned char* cannyEdgeDetection(const unsigned char* gray, int w, int h, unsigned char lowThresh, unsigned char highThresh)
{
    unsigned char* blurred   = new unsigned char[w * h];
    int*           Gx        = new int[w * h];
    int*           Gy        = new int[w * h];
    unsigned char* magnitude = new unsigned char[w * h];
    unsigned char* nms       = new unsigned char[w * h];
    unsigned char* dt        = new unsigned char[w * h];

    gaussianBlur(gray, blurred, w, h);

    sobel(blurred, Gx, Gy, w, h);

    for (int i = 0; i < w * h; i++) {
        int gx = Gx[i];
        int gy = Gy[i];
        int mag = (int)std::sqrt((double)gx * gx + (double)gy * gy);
        magnitude[i] = (unsigned char)clip(mag);
    }

    nonMaxSuppression(magnitude, Gx, Gy, nms, w, h);

    doubleThreshold(nms, dt, w, h, lowThresh, highThresh);

    hysteresis(dt, w, h);

    delete[] blurred;
    delete[] Gx;
    delete[] Gy;
    delete[] magnitude;
    delete[] nms;

    return dt;
}


unsigned char* makeHalftone(const unsigned char* gray, int grayWidth, int grayHeight, int& outWidth, int& outHeight) {
    outWidth  = grayWidth * 2;
    outHeight = grayHeight * 2;

    unsigned char* halftone = (unsigned char*)malloc(outWidth * outHeight * sizeof(unsigned char));
    if (!halftone)
        return nullptr;

    for (int y = 0; y < grayHeight; y++) {
        for (int x = 0; x < grayWidth; x++) {
            unsigned char g = gray[y * grayWidth + x]; // 0..255

            int level = (g * 5) / 256; // 0..4
            int dots  = 4 - level;     // how many black pixels

            int ox = 2 * x;
            int oy = 2 * y;

            int idx00 = (oy + 0) * outWidth + (ox + 0);
            int idx01 = (oy + 0) * outWidth + (ox + 1);
            int idx10 = (oy + 1) * outWidth + (ox + 0);
            int idx11 = (oy + 1) * outWidth + (ox + 1);

            halftone[idx00] = 255;
            halftone[idx01] = 255;
            halftone[idx10] = 255;
            halftone[idx11] = 255;

            if (dots >= 1) halftone[idx00] = 0;
            if (dots >= 2) halftone[idx11] = 0;
            if (dots >= 3) halftone[idx01] = 0;
            if (dots >= 4) halftone[idx10] = 0;
        }
    }

    return halftone;
}

unsigned char* floydSteinberg16(const unsigned char* gray, int w, int h) {
    std::vector<float> work(w * h);
    for (int i = 0; i < w * h; i++)
        work[i] = gray[i];

    unsigned char* out = new unsigned char[w * h];

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int idx = y * w + x;
            float oldVal = work[idx];

            int level = (int)std::round(oldVal / 255.0f * 15.0f); // 0..15
            float newVal = level * (255.0f / 15.0f);

            out[idx] = (unsigned char)clip((int)std::round(newVal));

            float err = oldVal - newVal;

            struct Nbr { int dx, dy; float w; };
            Nbr neighs[4] = {
                { +1,  0, 7.0f/16.0f },   // right
                { -1, +1, 3.0f/16.0f },   // down-left
                {  0, +1, 5.0f/16.0f },   // down
                { +1, +1, 1.0f/16.0f }    // down-right
            };

            for (int k = 0; k < 4; k++) {
                int nx = x + neighs[k].dx;
                int ny = y + neighs[k].dy;
                if (nx < 0 || nx >= w || ny < 0 || ny >= h) continue;
                int nidx = ny * w + nx;
                work[nidx] += err * neighs[k].w;
            }
        }
    }

    return out;
}

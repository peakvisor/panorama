#include <iostream>
#include <math.h>
#include <algorithm>
#include <string>
#include <unistd.h>

// CImg library
#include "CImg/CImg.h"
using namespace cimg_library;

//#include <xmmintrin.h>
//#include <pmmintrin.h>

// Multithreading
#include <tbb/parallel_for.h>
#include <tbb/blocked_range.h>
using namespace tbb;

// Input parameters
int iflag, oflag, hflag, rflag;
char *ivalue, *ovalue;
int rvalue=4096;

/**
 **	Parse input parameters
 **/
int parseParameters(int argc, char *argv[]) {
    iflag = oflag = hflag = rflag = 0;
    ivalue = ovalue = NULL;
    int c;
    opterr = 0;
    
    while ((c = getopt (argc, argv, "i:o:r:")) != -1)
        switch (c) {
            case 'i':
                // input file
                iflag = 1;
                ivalue = optarg;
                break;
            case 'o':
                oflag = 1;
                ovalue = optarg;
                break;
            case 'r':
                rflag = 1;
                rvalue = std::stoi(optarg);
                break;
            case '?':
                if (optopt == 'i' || optopt == 'o' || optopt == 'r')
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                else if (isprint (optopt))
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                return 1;
            default:
                abort ();
        }
    
    if (iflag==0 || oflag == 0) {
        std::cout << "No inputs or outputs specified: "<< iflag << "/" << oflag <<"\n";
        abort ();
        return 1;
    }
    return 0;
}

template <typename Coordinate>
struct Vec3_ {
    Coordinate x, y, z;
    
    Vec3_(Coordinate x = {}, Coordinate y = {}, Coordinate z = {})
        : x{x}, y{y}, z{z}
    {}
    
    template <typename Other>
    Vec3_(const Vec3_<Other> &other)
        : x{static_cast<Coordinate>(other.x)}
        , y{static_cast<Coordinate>(other.y)}
        , z{static_cast<Coordinate>(other.z)}
    {}
    
    inline Vec3_ operator +(const Vec3_ &other) const {
        return {x + other.x, y + other.y, z + other.z};
    }
    inline Vec3_ operator -(const Vec3_ &other) const {
        return {x - other.x, y - other.y, z - other.z};
    }
    inline Vec3_ operator *(Coordinate c) const {
        return {x * c, y * c, z * c};
    }
};
using Vec3fa = Vec3_<double>; // though not f(float) and not a(aligned)
using Vec3uc = Vec3_<unsigned char>;
struct PixelRange {int start, end; };

/** get x,y,z coords from out image pixels coords
 **	i,j are pixel coords
 **	face is face number
 **	edge is edge length
 **/
Vec3fa outImgToXYZ(int, int, int, int);
Vec3uc interpolateXYZtoColor(Vec3fa, CImg<unsigned char>&);
/**
 **	Convert panorama using an inverse pixel transformation
 **/
void convertBack(CImg<unsigned char>&, CImg<unsigned char> **);

int main (int argc, char *argv[]) {
    std::cout << "PeakVisor panorama translator...\n";
    
    parseParameters(argc, argv);
    
    std::cout << "  convert equirectangular panorama: [" << ivalue << "] into cube faces: ["<< ovalue << "] of " << rvalue <<" pixels in dimension\n";
    
    // Input image
    CImg<unsigned char> imgIn(ivalue);
    
    // Create output images
    CImg<unsigned char>* imgOut[6];
    for (int i=0; i<6; ++i){
        imgOut[i] = new CImg<unsigned char>(rvalue, rvalue, 1, 4, 255);
    }
    
    // Convert panorama
    convertBack(imgIn, imgOut);
    
    // Write output images
    for (int i=0; i<6; ++i){
        std::string fname = std::string(ovalue) + "_" + std::to_string(i) + ".jpg";//".jpg";
        imgOut[i]->save_jpeg( fname.c_str(), 85);
//        imgOut[i]->save_png(fname.c_str());
    }
    
    std::cout << "  convertation finished successfully\n";
    return 0;
}



/**
 **	Convert panorama using an inverse pixel transformation
 **/
void convertBack(CImg<unsigned char>& imgIn, CImg<unsigned char> **imgOut){
    int _dw = rvalue*4;
    int edge = rvalue; // the length of each edge in pixels
    int face = 0;
    
    // Look around cube faces
//    for (int i = 0; i < _dw; ++i) {
    tbb::parallel_for(blocked_range<size_t>(0, _dw, 1),
                              [&](const blocked_range<size_t>& range) {
//        for (size_t i=/*range.begin()*/0; i</*range.end()*/_dw; ++i) {
                                  int face = 0;
                    for (size_t i=range.begin(); i<range.end(); ++i) {
            face = int(i/edge); // 0 - back, 1 - left 2 - front, 3 - right
            PixelRange rng = {edge, 2*edge};
            
            if (i>=2*edge && i<3*edge) {
                rng = {0, 3*edge};
            }
            
            for (int j=rng.start; j<rng.end; ++j) {
                if (j<edge) {
                    face = 4;
                } else if (j>2*edge) {
                    face = 5;
                } else {
                    face = int(i/edge);
                }
                
                Vec3fa xyz = outImgToXYZ(i, j, face, edge);
                Vec3uc clr = interpolateXYZtoColor(xyz, imgIn);
                const unsigned char color[] = { clr.x, clr.y, clr.z, 255 };
                imgOut[face]->draw_point(i%edge, j%edge, 0, color);
            }
//                    }
//                              }();
                    }
                              });
    /*for (int i=0; i<_dw; ++i) {
     face = int(i/edge); // 0 - back, 1 - left 2 - front, 3 - right
     Range rng = {edge, 2*edge};
     
     if (i>=2*edge && i<3*edge) {
     rng = {0, 3*edge};
     }
     
     for (int j=rng.start; j<rng.end; ++j) {
     if (j<edge) {
     face = 4;
     } else if (j>2*edge) {
     face = 5;
     } else {
     face = int(i/edge);
     }
     
     Vec3fa xyz = outImgToXYZ(i, j, face, edge);
     Vec3uc clr = interpolateXYZtoColor(xyz, imgIn);
     const unsigned char color[] = { clr.x, clr.y, clr.z, 255 };
     imgOut[face]->draw_point(i%edge, j%edge, 0, color);
     }
     
     }*/
}

Vec3fa outImgToXYZ(int i, int j, int face, int edge) {
    auto a = 2.0 * i / edge;
    auto b = 2.0 * j / edge;
    Vec3fa res;
    if (face == 0) { // back
        res = {-1, 1 - a, 3 - b};
    } else if (face == 1) { // left
        res = {a - 3, -1, 3 - b};
    } else if (face == 2) { // front
        res = {1, a - 5, 3 - b};
    } else if (face == 3) { // right
        res = {7 - a, 1, 3 - b};
    } else if (face == 4) { // top
        res = {b - 1, a - 5, 1};
    } else if (face==5) { // bottom
        res = {5 - b, a - 5, -1};
    }
    return res;
}

template <typename T>
static inline T clamp(const T &n, const T &lower, const T &upper) {
    return std::min(std::max(n, lower), upper);
}

template <typename T>
static inline T safeIndex(const T n, const T size) {
    return clamp(n, {}, size - 1);
}

template <typename T, typename Scalar>
static inline T mix(const T &one, const T &other, const Scalar &c) {
    return one + (other - one) * c;
}

Vec3uc interpolateXYZtoColor(Vec3fa xyz, CImg<unsigned char>& imgIn) {
    auto _sw = imgIn.width(), _sh = imgIn.height();
    
    auto theta = std::atan2(xyz.y, xyz.x), r = std::hypot(xyz.x, xyz.y);// # range -pi to pi
    auto phi = std::atan2(xyz.z, r);// # range -pi/2 to pi/2
    
    // source img coords
    auto uf = (theta + M_PI) / M_PI * _sh;
    auto vf = (M_PI_2 - phi) / M_PI * _sh; // implicit assumption: _sh == _sw / 2
    // Use bilinear interpolation between the four surrounding pixels
    auto ui = safeIndex(static_cast<int>(std::floor(uf)), _sw);
    auto vi = safeIndex(static_cast<int>(std::floor(vf)), _sh);  //# coord of pixel to bottom left
    auto u2 = safeIndex(ui + 1, _sw);
    auto v2 = safeIndex(vi + 1, _sh);       //# coords of pixel to top right
    double mu = uf - ui, nu = vf - vi;      //# fraction of way across pixel
    mu = nu = 0;
    
    // Pixel values of four nearest corners
    auto read = [&](int x, int y) { return Vec3fa{Vec3uc{*imgIn.data(x, y, 0, 0), *imgIn.data(x, y, 0, 1), *imgIn.data(x, y, 0, 2)}}; };
    auto A = read(ui, vi), B = read(u2, vi), C = read(ui, v2), D = read(u2, v2);
    
    // Interpolate color
    auto value = mix(mix(A, B, mu), mix(C, D, mu), nu);
    return Vec3uc{value};
}

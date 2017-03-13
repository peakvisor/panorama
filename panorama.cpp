#include <iostream>
#include <math.h> 
#include <algorithm>
#include <string> 
#include <unistd.h>

#include "CImg/CImg.h"
using namespace cimg_library;

int iflag, oflag, hflag, rflag;
char *ivalue, *ovalue;
int rvalue=4096;

/**
**	Parse input parameters 
**/
int parseParameters(int argc, char *argv[]) {
	iflag = oflag = hflag = rflag = 0;
	ivalue = ovalue = NULL;
	int index;
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

struct Vec3fa { double x, y, z; };
struct Vec3uc { unsigned char x, y, z; };
struct Range {int start, end; };

/** get x,y,z coords from out image pixels coords
**	i,j are pixel coords
**	face is face number
**	edge is edge length
**/
Vec3fa outImgToXYZ(int i, int j, int face, int edge) {
    float a = 2.0f*float(i)/edge;
    float b = 2.0f*float(j)/edge;
    Vec3fa res;
    if (face==0) { // back
        res = {-1.0f, 1.0f-a, 3.0f - b};
    } else if (face==1) { // left
        res = {a-3.0f, -1.0f, 3.0f - b};
    } else if (face==2) { // front
        res = {1.0f, a - 5.0f, 3.0f - b};
    } else if (face==3) { // right
        res = {7.0f-a, 1.0f, 3.0f - b};
    } else if (face==4) { // top
        res = {b-1.0f, a -5.0f, 1.0f};
    } else if (face==5) { // bottom
        res = {5.0f-b, a-5.0f, -1.0f};
    }
    return res;
}

/**	
**	Clip a value into boundaries
**/
template <typename T>
T clip(const T& n, const T& lower, const T& upper) {
  return std::max(lower, std::min(n, upper));
}

/** 
**	Convert panorama using an inverse pixel transformation
**/
void convertBack(CImg<unsigned char>& imgIn, CImg<unsigned char> **imgOut){
    int _sw = imgIn.width();
    int _sh = imgIn.height();
    int _dw = rvalue*4;
    int edge = rvalue; // the length of each edge in pixels

    for (int i=0; i<_dw; ++i) {
		int face = int(i/edge); // 0 - back, 1 - left 2 - front, 3 - right
		Range rng = {edge, 2*edge};

        for (int j=rng.start; j<rng.end; ++j) {

			Vec3fa xyz = outImgToXYZ(i, j, face, edge);
			double theta = atan2(xyz.y, xyz.x);// # range -pi to pi
			double r = hypot(xyz.x, xyz.y);
			double phi = atan2(xyz.z, r);// # range -pi/2 to pi/2

			// source img coords
			double uf = ( 2.0 *edge* (theta + M_PI)/ M_PI)*_sw/(4*edge);
			double vf = ( 2.0 *edge* (M_PI/2 - phi)/ M_PI)*_sw/(4*edge);
			// Use bilinear interpolation between the four surrounding pixels
			int ui = floor(uf);  //# coord of pixel to bottom left
			int vi = floor(vf);
			int u2 = ui+1;       //# coords of pixel to top right
			int v2 = vi+1;
			double mu = uf-ui;      //# fraction of way across pixel
			double nu = vf-vi;
			// Pixel values of four corners
			Vec3uc A = {*imgIn.data(ui % _sw, clip(vi,0,_sh-1), 0, 0), *imgIn.data(ui % _sw, clip(vi,0,_sh-1), 0, 1), *imgIn.data(ui % _sw, clip(vi,0,_sh-1), 0, 2)};
			Vec3uc B = {*imgIn.data(u2 % _sw, clip(vi,0,_sh-1), 0, 0), *imgIn.data(u2 % _sw, clip(vi,0,_sh-1), 0, 1), *imgIn.data(u2 % _sw, clip(vi,0,_sh-1), 0, 2)};
			Vec3uc C = {*imgIn.data(ui % _sw, clip(v2,0,_sh-1), 0, 0), *imgIn.data(ui % _sw, clip(v2,0,_sh-1), 0, 1), *imgIn.data(ui % _sw, clip(v2,0,_sh-1), 0, 2)};
			Vec3uc D = {*imgIn.data(u2 % _sw, clip(v2,0,_sh-1), 0, 0), *imgIn.data(u2 % _sw, clip(v2,0,_sh-1), 0, 1), *imgIn.data(u2 % _sw, clip(v2,0,_sh-1), 0, 2)};
			// interpolate
			unsigned char rc = A.x*(1-mu)*(1-nu) + B.x*(mu)*(1-nu) + C.x*(1-mu)*nu+D.x*mu*nu;
			unsigned char gc = A.y*(1-mu)*(1-nu) + B.y*(mu)*(1-nu) + C.y*(1-mu)*nu+D.y*mu*nu;
			unsigned char bc = A.z*(1-mu)*(1-nu) + B.z*(mu)*(1-nu) + C.z*(1-mu)*nu+D.z*mu*nu;

			const unsigned char color[] = { rc, gc, bc, 255 };
			imgOut[face]->draw_point(i%edge, j%edge, 0, color);
        }
    }
}

int main (int argc, char *argv[]) {
	std::cout << "PeakVisor panorama translator...\n";

	parseParameters(argc, argv);

	std::cout << "  convert equirectangular panorama: [" << ivalue << "] into cube faces: ["<< ovalue << "] of " << rvalue <<" pixels in dimension\n";

	// Input image
	CImg<unsigned char> imgIn(ivalue);

	// Output images
	CImg<unsigned char>* imgOut[4];
	for (int i=0; i<4; ++i){
		imgOut[i] = new CImg<unsigned char>(rvalue, rvalue, 1, 4, 255);
	}
	
	convertBack(imgIn, imgOut);

	// Look around cube faces
	for (int i=0; i<4; ++i){
		std::string fname = std::string(ovalue) + "_" + std::to_string(i) + ".jpg";
		imgOut[i]->save_jpeg( fname.c_str(), 85);
	}
	// Top and bottom faces
	

	//imgOut.save_jpeg( "./result.jpg", 85);
}
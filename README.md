# Photo Panorama Converter

This application converts an equirectangular panorama image into 6 cube faces images. The tool might be very handy when you need to prepare a cube faces panorama for some of web panorama viewers (a.e. Panellum).

This tool was developed as part of the [PeakVisor](http://peakvisor.com "PeakVisor") service. Please check it out, it is fantastic! Here is the webpage which renders [mountain panoramas](http://peakvisor.com/panorama.html "Mountain Panoramas")

# How to convert an equirectangular panorama?

Launch the panorama utility with following parameters:
-i - equirectangular panorama source
-o - output cube faces names
-r - edge length of a cube face (optional)

```
panorama -i ./samples/equirectangular_panorama.jpg -o cube_faces -r 4096
```

For a test you might take the panoramic photo from the samples directory. It was taken with a Panono 360-degrees camera (dimensions 16384x8192) on the way to the [Monte Bregagnino](https://peakvisor.com/peak/bregagnino.html)'s summit (Lake Como, Italy). Depending on your CPU performance it might take from several seconds to a minute.

Here is a sample webGL panorama viewer based on threeJS (LINK)

# How does the conversion work?

For a detailed description of the algorithm (and geometry) behind the tool please refer to this [StackOverflow thread](http://stackoverflow.com/questions/29678510/convert-21-equirectangular-panorama-to-cube-map). Basically it goes through all the pixels of the target image and calculates the related pixel (or it's approximation) in the source panorama.

Obviously, it decreases the panorama's quality and you'd better avoid this transformation.

# Requirements

The program requires the Intel [Threading Building Blocks](https://www.threadingbuildingblocks.org/) (TBB), [libpng](http://www.libpng.org/pub/png/libpng.html), and [libjpeg](http://libjpeg.sourceforge.net/) libraries.

On ubuntu these can be installed with 
```
sudo apt-get install libtbb-dev libpng-dev libjpeg-dev
```

# How to build the panorama converter

Clone the repository with:
```
git clone
```

Initialize dependencies:
```
git submodule init
git submodule update
```

Check other dependencies (JPG, PNG, Intel TBB). Update paths in the makefile if needed.

Make the panorama tool
```
make
```

That's it!

# How to use panorama converter as a web service

We're running this panorama converter on some of our servers and if you are interested in using it as a web service then please let me know: denis@denivip.ru

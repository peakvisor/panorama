# panorama
panorama converter

This application converts equirectangular panorama image into 6 cube faces. This tool might be very handy when you need to prepare a cube panorama for most of web panorama viewers. 

This tool was developed as part of the [PeakVisor](http://peakvisor.com "PeakVisor") service. Please check it out, it is wonderful! Here is the webpage which renders [mountain panoramas](http://peakvisor.com/panorama.html "Mountain Panoramas")

# How to convert an equirectangular panorama?

panorama -i er_panorama.jpg -o cube_faces.jpg -r 4096

# Hot to build the panorama converter

Clone the repository with:
git clone

Initialize dependencies:
git submodule init
git submovulde update

Check other dependencies (JPG, PNG, X11)
Update paths in the makefile if needed

Make the tool
make

That's it
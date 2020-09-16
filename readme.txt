Paint++: an MSPaint clone that supports transparency and animated formats using FFmpeg.


How to compile:
	Compile ppp.cpp with stb_image and lodepng libraries.

How to install:
	1) Install Microsoft Visual C++ 2013 Redistributable (x86) if not installed:
		https://www.microsoft.com/en-us/download/details.aspx?id=40784
	2) Download FFmpeg for Windows from:
		https://ffmpeg.org/download.html
	3) Open Paint++ and paste the path to the extracted FFmpeg executables into the command window.

How to use:
	Press F1 for help.
	When opening a file, Paint++ creates a temporary directory (in the same folder with the executable)
	with the frame(s) converted to a temporary format (PNG by default). When closing the program it asks
	if you want to delete the temporary directory.
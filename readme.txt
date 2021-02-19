Paint++: an MSPaint clone that supports transparency and animated formats using FFmpeg.


How to compile:
	Compile all sources with the lodepng and stb_image libraries
	as a 32-bit Windows application
	using Visual C++ 2010 Express or later.

lodepng by Lode Vandevenne
https://github.com/lvandeve/lodepng

stb_image by Sean Barrett
https://github.com/nothings/stb/blob/master/stb_image.h


How to install:
	1) Install Microsoft Visual C++ Redistributable (x86), if not installed.
	Select the runtime library version:

		Microsoft Visual C++ 2010 Redistributable (msvcp/msvcr100)
		https://www.microsoft.com/en-eg/download/details.aspx?id=5555

		Microsoft Visual C++ 2013 Redistributable (msvcp/msvcr120)
		https://www.microsoft.com/en-us/download/details.aspx?id=40784

	2) Download FFmpeg for Windows from:
		https://ffmpeg.org/download.html

	3) Open Paint++ and paste the path to the extracted FFmpeg executables into the command window.

How to use:
	Press F1 for help.
	When opening a file, Paint++ creates a temporary directory (in the same folder with the executable)
	with the frame(s) converted to a temporary format (PNG by default). When closing the program it asks
	if you want to delete the temporary directory.
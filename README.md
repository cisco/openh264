OpenH264
========
OpenH264 is a codec library which supports H.264 encoding and decoding. It is suitable for use in real time applications such as WebRTC. See http://www.openh264.org/ for more details.

Encoder Features
----------------
- Constrained Baseline Profile up to Level 5.2 (4096x2304)
- Arbitrary resolution, not constrained to multiples of 16x16
- Rate control with adaptive quantization, or constant quantization
- Slice options: 1 slice per frame, N slices per frame, N macroblocks per slice, or N bytes per slice
- Multiple threads automatically used for multiple slices
- Temporal scalability up to 4 layers in a dyadic hierarchy
- Spatial simulcast up to 4 resolutions from a single input
- Long Term Reference (LTR) frames
- Memory Management Control Operation (MMCO)
- Reference picture list modification
- Single reference frame for inter prediction
- Multiple reference frames when using LTR and/or 3-4 temporal layers
- Periodic and on-demand Instantaneous Decoder Refresh (IDR) frame insertion
- Dynamic changes to bit rate, frame rate, and resolution
- Annex B byte stream output
- YUV 4:2:0 planar input

Decoder Features
----------------
- Constrained Baseline Profile up to Level 5.2 (4096x2304)
- Arbitrary resolution, not constrained to multiples of 16x16
- Single thread for all slices
- Long Term Reference (LTR) frames
- Memory Management Control Operation (MMCO)
- Reference picture list modification
- Multiple reference frames when specified in Sequence Parameter Set (SPS)
- Annex B byte stream input
- YUV 4:2:0 planar output

OS Support
----------
- Windows 64-bit and 32-bit
- Mac OS X 64-bit and 32-bit
- Linux 64-bit and 32-bit
- Android 32-bit (initial release does not include this target, will follow soon)
- iOS 64-bit and 32-bit (not supported yet, may be added in the future)

Processor Support
-----------------
- Intel x86 optionally with MMX/SSE (no AVX yet, help is welcome)
- ARMv7 optionally with NEON (initial release does not include this target, will follow later)
- Any architecture using C/C++ fallback functions

Building the Library
--------------------
NASM needed to be installed for assembly code: workable version 2.07 or above, nasm can downloaded from http://www.nasm.us/

For Windows Builds
------------------

Our Windows builds use MinGW which can be found here - http://www.mingw.org/

To build with gcc, add the MinGW bin directory (e.g. /c/MinGW/bin) to your path and follow the 'For All Platforms' instructions below.

To build with Visual Studio you will need to setup your path to run cl.exe.  Here is an example from a Windows 64bit install of VS2012:

export PATH="$PATH:/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin:/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE"

You will also need to set your INCLUDE and LIB paths to point to your VS and SDK installs.  Something like this, again from Win64 with VS2012 (note the use of Windows-style paths here).

export INCLUDE="C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\include;C:\Program Files (x86)\Windows Kits\8.0\Include\um;C:\Program Files (x86)\Windows Kits\8.0\Include\shared"
export LIB="C:\Program Files (x86)\Windows Kits\8.0\Lib\Win8\um\x86;C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\lib"

Then add 'UNAME=msvc' to the make line of the 'For All Platforms' instructions.

For All Platforms
-------------------
From the main project directory:
'make' for 32bit builds
'make ENABLE64BIT=Yes' for 64bit builds

The command line programs h264enc and h264dec will appear in the main project directory.

A shell script to run the command-line apps is in testbin/CmdLineExample.sh

Usage information can be found in testbin/CmdLineReadMe

Using the Source
----------------
codec - encoder, decoder, console (test app), build (makefile, vcproj)
processing - raw pixel processing (used by encoder)
build - scripts for Makefile build system.
test - GTest unittest files.
testbin - autobuild scripts, test app config files, yuv test files
bin - binaries for library and test app

Known Issues
------------
See the issue tracker on https://github.com/cisco/openh264/issues
- Encoder errors when resolution exceeds 3840x2160
- Encoder errors when compressed frame size exceeds half uncompressed size
- Decoder errors when compressed frame size exceeds 1MB
- Encoder RC requires frame skipping to be enabled to hit the target bitrate,
  if frame skipping is disabled the target bitrate may be exceeded

License
-------
BSD, see LICENSE file for details.

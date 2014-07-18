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
- Android 32-bit
- iOS 64-bit and 32-bit

Processor Support
-----------------
- Intel x86 optionally with MMX/SSE (no AVX yet, help is welcome)
- ARMv7 optionally with NEON
- Any architecture using C/C++ fallback functions

Building the Library
--------------------
NASM needed to be installed for assembly code: workable version 2.07 or above, nasm can downloaded from http://www.nasm.us/

To build the arm assembly for Windows Phone, gas-preprocessor is required. It can be downloaded from git://git.libav.org/gas-preprocessor.git

For Android Builds
------------------
To build for android platform, You need to install android sdk and ndk. You also need to export **ANDROID_SDK**/tools to PATH. On Linux, this can be done by

'export PATH=**ANDROID_SDK**/tools:$PATH'

The codec and demo can be built by

'make OS=android NDKROOT=**ANDROID_NDK** TARGET= **ANDROID_TARGET**'

Valid **ANDROID_TARGET** can be found in **ANDROID_SDK**/platforms, such as android-12.
You can also set ARCH, NDKLEVEL, GCCVERSION according to your device and NDK version.
ARCH specifies the architecture of android device. Currently only arm and x86 are supported, the default is arm.
NDKLEVEL specifies android api level, the api level can be 12-19, the default is 12.

By default these commands build for the armeabi-v7a ABI. To build for the other android
ABIs, add "ARCH=arm64", "ARCH=x86", "ARCH=x86_64", "ARCH=mips" or "ARCH=mips64".
To build for the older armeabi ABI (which has armv5te as baseline), add "APP_ABI=armeabi" (ARCH=arm is implicit).

For iOS Builds
--------------
You can build the libraries and demo applications using xcode project files
located in codec/build/iOS/dec and codec/build/iOS/enc.

You can also build the libraries (but not the demo applications) using the
make based build system from the command line. Build with

'make OS=ios ARCH=**ARCH**'

Valid values for **ARCH** are the normal iOS architecture names such as
armv7, armv7s, arm64, and i386 and x86_64 for the simulator. Additionally,
one might need to add 'SDK=X.Y' to the make command line in case the default
SDK version isn't available. Another settable iOS specific parameter
is SDK_MIN, specifying the minimum deployment target for the built library.
For other details on building using make on the command line, see
'For All Platforms' below.

For Windows Builds
------------------

Our Windows builds use MinGW which can be found here - http://www.mingw.org/

To build with gcc, add the MinGW bin directory (e.g. /c/MinGW/bin) to your path and follow the 'For All Platforms' instructions below.

To build with Visual Studio you will need to set up your path to run cl.exe.  The easiest way is to start MSYS from a developer command line session - http://msdn.microsoft.com/en-us/library/ms229859(v=vs.110).aspx  If you need to do it by hand here is an example from a Windows 64bit install of VS2012:

export PATH="$PATH:/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin:/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE"

You will also need to set your INCLUDE and LIB paths to point to your VS and SDK installs.  Something like this, again from Win64 with VS2012 (note the use of Windows-style paths here).

export INCLUDE="C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\include;C:\Program Files (x86)\Windows Kits\8.0\Include\um;C:\Program Files (x86)\Windows Kits\8.0\Include\shared"
export LIB="C:\Program Files (x86)\Windows Kits\8.0\Lib\Win8\um\x86;C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\lib"

Then add 'OS=msvc' to the make line of the 'For All Platforms' instructions.

For All Platforms
-------------------
From the main project directory:
'make' for automatically detecting 32/64bit and building accordingly
'make ENABLE64BIT=No' for 32bit builds
'make ENABLE64BIT=Yes' for 64bit builds
'make V=No' for a silent build (not showing the actual compiler commands)

The command line programs h264enc and h264dec will appear in the main project directory.

A shell script to run the command-line apps is in testbin/CmdLineExample.sh

Usage information can be found in testbin/CmdLineReadMe

Using the Source
----------------
codec - encoder, decoder, console (test app), build (makefile, vcproj)
build - scripts for Makefile build system.
test - GTest unittest files.
testbin - autobuild scripts, test app config files
res - yuv and bitstream test files

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

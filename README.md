OpenH264
=======
OpenH264 is a codec library which supports H.264 encoding and decoding. It is suitable for use in real time applications such as WebRTC. See http://www.openh264.org/ for more details.
 
Encoder Features
------------------------
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
------------------------
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
----------------
- Windows 64-bit and 32-bit (initial release is only 32-bit, 64-bit will follow soon)
- Mac OS X 64-bit (initial release does not include this target, will follow soon)
- Linux 64-bit and 32-bit (initial release is only 32-bit, 64-bit will follow soon)
- Android 32-bit (initial release does not include this target, will follow soon)
- iOS 64-bit and 32-bit (not supported yet, may be added in the future)
 
Processor Support
-------------------------
- Intel x86 optionally with MMX/SSE (no AVX yet, help is welcome)
- ARMv7 optionally with NEON (initial release does not include this target, will follow later)
- Any architecture using C/C++ fallback functions

Using the Library
-----------------------
Linux makefiles for 32 bit builds are available:
    : build the decoder library and executable via codec/build/linux/dec/makefile
    : build the encoder library and executable via codec/build/linux/enc/makefile
    : build the encoder shared library via processing/build/linux/makefile
 
Windows Visual Studio 2008/2010/2012 projects are available:
    : build the decoder via the Visual Studio projects in codec/build/win32/dec
    : build the encoder via the Visual Studio projects in codec/build/win32/dec
    : build the encoder shared library via the Visual Studio projects in processing/build/win32/
 
NASM needed to be installed for assembly code: workable version 2.07 or above, nasm can downloaded from http://www.nasm.us/
 
API details to be provided later.
 
Using the Test App
-------------------------
Linux shell scripts to build the test apps:
    : build via testbin/AutoBuild_Linux.sh
    : clean via testbin/AutoClean_Linux.sh
 
Windows batch files to build the test apps:
    : Visual Studio 2008 use testbin/AutoBuild_Windows_VS2008.bat
    : Visual Studio 2010 use testbin/AutoBuild_Windows_VS2010.bat
    : Visual Studio 2012 use testbin/AutoBuild_Windows_VS2012.bat
 
Usage information can be found in testbin/CmdLineReadMe
Command line options and details to be provided later.
 
Using the Source
-----------------------
codec - encoder, decoder, console (test app), build (makefile, vcproj)
processing - raw pixel processing (used by encoder)
testbin - autobuild scripts, test app config files, yuv test files
bin - binaries for library and test app
 
Known Issues
-------------------
See the issue tracker on https://github.com/cisco/openh264/issues
- Encoder errors when resolution exceeds 3840x2160
- Encoder errors when compressed frame size exceeds half uncompressed size
- Encoder console app only support multiple of 16 width/height for now
- Decoder errors when compressed frame size exceeds 1MB
 
License
----------
BSD, see LICENSE file for details.

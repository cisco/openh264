OpenH264. Rolando Gopez Lacuata.
========
OpenH264 is a codec library which supports H.264 encoding and decoding. It is suitable for use in real time applications such as WebRTC. See http://www.openh264.org/ for more details.

Encoder Features
----------------
- Constrained Baseline Profile up to Level 5.2 (Max frame size is 36864 macro-blocks)
- Arbitrary resolution, not constrained to multiples of 16x16
- Rate control with adaptive quantization, or constant quantization
- Slice options: 1 slice per frame, N slices per frame, N macroblocks per slice, or N bytes per slice
- Multiple threads automatically used for multiple slices
- Temporal scalability up to 4 layers in a dyadic hierarchy
- Simulcast AVC up to 4 resolutions from a single input
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
- Constrained Baseline Profile up to Level 5.2 (Max frame size is 36864 macro-blocks)
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
- Android 64-bit and 32-bit
- iOS 64-bit and 32-bit
- Windows Phone 32-bit

Processor Support
-----------------
- Intel x86 optionally with MMX/SSE (no AVX yet, help is welcome)
- ARMv7 optionally with NEON, AArch64 optionally with NEON
- Any architecture using C/C++ fallback functions

Building the Library
--------------------
NASM needed to be installed for assembly code: workable version 2.10.06 or above, NASM can downloaded from http://www.nasm.us/.
For Mac OSX 64-bit NASM needed to be below version 2.11.08 as NASM 2.11.08 will introduce error when using RIP-relative addresses in Mac OSX 64-bit

To build the arm assembly for Windows Phone, gas-preprocessor is required. It can be downloaded from git://git.libav.org/gas-preprocessor.git

For Android Builds
------------------
To build for android platform, You need to install android sdk and ndk. You also need to export `**ANDROID_SDK**/tools` to PATH. On Linux, this can be done by

    export PATH=**ANDROID_SDK**/tools:$PATH

The codec and demo can be built by

    make OS=android NDKROOT=**ANDROID_NDK** TARGET=**ANDROID_TARGET**

Valid `**ANDROID_TARGET**` can be found in `**ANDROID_SDK**/platforms`, such as `android-12`.
You can also set `ARCH`, `NDKLEVEL` according to your device and NDK version.
`ARCH` specifies the architecture of android device. Currently `arm`, `arm64`, `x86` and `x86_64` are supported, the default is `arm`. (`mips` and `mips64` can also be used, but there's no specific optimization for those architectures.)
`NDKLEVEL` specifies android api level, the default is 12. Available possibilities can be found in `**ANDROID_NDK**/platforms`, such as `android-21` (strip away the `android-` prefix).

By default these commands build for the `armeabi-v7a` ABI. To build for the other android
ABIs, add `ARCH=arm64`, `ARCH=x86`, `ARCH=x86_64`, `ARCH=mips` or `ARCH=mips64`.
To build for the older `armeabi` ABI (which has armv5te as baseline), add `APP_ABI=armeabi` (`ARCH=arm` is implicit).
To build for 64-bit ABI, such as `arm64`, explicitly set `NDKLEVEL` to 21 or higher.

For iOS Builds
--------------
You can build the libraries and demo applications using xcode project files
located in `codec/build/iOS/dec` and `codec/build/iOS/enc`.

You can also build the libraries (but not the demo applications) using the
make based build system from the command line. Build with

    make OS=ios ARCH=**ARCH**

Valid values for `**ARCH**` are the normal iOS architecture names such as
`armv7`, `armv7s`, `arm64`, and `i386` and `x86_64` for the simulator.
Another settable iOS specific parameter
is `SDK_MIN`, specifying the minimum deployment target for the built library.
For other details on building using make on the command line, see
'For All Platforms' below.

For Windows Builds
------------------

Our Windows builds use MinGW which can be downloaded from http://www.mingw.org/

To build with gcc, add the MinGW bin directory (e.g. `/c/MinGW/bin`) to your path and follow the 'For All Platforms' instructions below.

To build with Visual Studio you will need to set up your path to run cl.exe.  The easiest way is to start MSYS from a developer command line session.  Instructions can be found at http://msdn.microsoft.com/en-us/library/ms229859(v=vs.110).aspx.  If you need to do it by hand here is an example from a Windows 64bit install of VS2012:

    export PATH="$PATH:/c/Program Files (x86)/Microsoft Visual Studio 11.0/VC/bin:/c/Program Files (x86)/Microsoft Visual Studio 11.0/Common7/IDE"

You will also need to set your INCLUDE and LIB paths to point to your VS and SDK installs.  Something like this, again from Win64 with VS2012 (note the use of Windows-style paths here).

    export INCLUDE="C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\include;C:\Program Files (x86)\Windows Kits\8.0\Include\um;C:\Program Files (x86)\Windows Kits\8.0\Include\shared"
    export LIB="C:\Program Files (x86)\Windows Kits\8.0\Lib\Win8\um\x86;C:\Program Files (x86)\Microsoft Visual Studio 11.0\VC\lib"

Then add `OS=msvc` to the make line of the 'For All Platforms' instructions.

For Windows Phone Builds
------------------------

Follow the instructions above for normal Windows builds, but use `OS=msvc-wp`
instead of `OS=msvc`. You will also need gas-preprocessor (as mentioned below
"Building the Library").

If building for Windows Phone with MSVC 2013, there's no included bat file that sets the lib paths to the Windows Phone kit, but that can be done with a command like this:

    export LIB="c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\store\arm;c:\Program Files (x86)\Microsoft Visual Studio 12.0\VC\lib\arm;c:\Program Files (x86)\Windows Phone Kits\8.1\lib\arm"

This is only necessary for building the DLL; the static library can be built without setting this.

Note, only Windows Phone 8.1 or newer is supported, 8.0 is no longer supported.

For All Platforms
-------------------

Using make
----------

From the main project directory:
- `make` for automatically detecting architecture and building accordingly
- `make ARCH=i386` for x86 32-bit builds
- `make ARCH=x86_64` for x86 64-bit builds
- `make V=No` for a silent build (not showing the actual compiler commands)
- `make DEBUGSYMBOLS=True` for two libraries, one is normal libraries, another one is removed the debugging symbol table entries (those created by the -g option)

The command line programs `h264enc` and `h264dec` will appear in the main project directory.

A shell script to run the command-line apps is in `testbin/CmdLineExample.sh`

Usage information can be found in `testbin/CmdLineReadMe`

Using meson
-----------

Meson build definitions have been added, and are known to work on Linux
and Windows, for x86 and x86 64-bit.

See <http://mesonbuild.com/Installing.html> for instructions on how to
install meson, then:

``` shell
meson builddir
ninja -C builddir
```

Run the tests with:

``` shell
meson test -C builddir -v
```

Install with:

``` shell
ninja -C builddir install
```

Using the Source
----------------
- `codec` - encoder, decoder, console (test app), build (makefile, vcproj)
- `build` - scripts for Makefile build system
- `test` - GTest unittest files
- `testbin` - autobuild scripts, test app config files
- `res` - yuv and bitstream test files

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

Apache License
                           Version 2.0, January 2004
                        https://www.apache.org/licenses/

   TERMS AND CONDITIONS FOR USE, REPRODUCTION, AND DISTRIBUTION

   1. Definitions.

      "License" shall mean the terms and conditions for use, reproduction,
      and distribution as defined by Sections 1 through 9 of this document.

      "Licensor" shall mean the copyright owner or entity authorized by
      the copyright owner that is granting the License.

      "Legal Entity" shall mean the union of the acting entity and all
      other entities that control, are controlled by, or are under common
      control with that entity. For the purposes of this definition,
      "control" means (i) the power, direct or indirect, to cause the
      direction or management of such entity, whether by contract or
      otherwise, or (ii) ownership of fifty percent (50%) or more of the
      outstanding shares, or (iii) beneficial ownership of such entity.

      "You" (or "Your") shall mean an individual or Legal Entity
      exercising permissions granted by this License.

      "Source" form shall mean the preferred form for making modifications,
      including but not limited to software source code, documentation
      source, and configuration files.

      "Object" form shall mean any form resulting from mechanical
      transformation or translation of a Source form, including but
      not limited to compiled object code, generated documentation,
      and conversions to other media types.

      "Work" shall mean the work of authorship, whether in Source or
      Object form, made available under the License, as indicated by a
      copyright notice that is included in or attached to the work
      (an example is provided in the Appendix below).

      "Derivative Works" shall mean any work, whether in Source or Object
      form, that is based on (or derived from) the Work and for which the
      editorial revisions, annotations, elaborations, or other modifications
      represent, as a whole, an original work of authorship. For the purposes
      of this License, Derivative Works shall not include works that remain
      separable from, or merely link (or bind by name) to the interfaces of,
      the Work and Derivative Works thereof.

      "Contribution" shall mean any work of authorship, including
      the original version of the Work and any modifications or additions
      to that Work or Derivative Works thereof, that is intentionally
      submitted to Licensor for inclusion in the Work by the copyright owner
      or by an individual or Legal Entity authorized to submit on behalf of
      the copyright owner. For the purposes of this definition, "submitted"
      means any form of electronic, verbal, or written communication sent
      to the Licensor or its representatives, including but not limited to
      communication on electronic mailing lists, source code control systems,
      and issue tracking systems that are managed by, or on behalf of, the
      Licensor for the purpose of discussing and improving the Work, but
      excluding communication that is conspicuously marked or otherwise
      designated in writing by the copyright owner as "Not a Contribution."

      "Contributor" shall mean Licensor and any individual or Legal Entity
      on behalf of whom a Contribution has been received by Licensor and
      subsequently incorporated within the Work.

   2. Grant of Copyright License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      copyright license to reproduce, prepare Derivative Works of,
      publicly display, publicly perform, sublicense, and distribute the
      Work and such Derivative Works in Source or Object form.

   3. Grant of Patent License. Subject to the terms and conditions of
      this License, each Contributor hereby grants to You a perpetual,
      worldwide, non-exclusive, no-charge, royalty-free, irrevocable
      (except as stated in this section) patent license to make, have made,
      use, offer to sell, sell, import, and otherwise transfer the Work,
      where such license applies only to those patent claims licensable
      by such Contributor that are necessarily infringed by their
      Contribution(s) alone or by combination of their Contribution(s)
      with the Work to which such Contribution(s) was submitted. If You
      institute patent litigation against any entity (including a
      cross-claim or counterclaim in a lawsuit) alleging that the Work
      or a Contribution incorporated within the Work constitutes direct
      or contributory patent infringement, then any patent licenses
      granted to You under this License for that Work shall terminate
      as of the date such litigation is filed.

   4. Redistribution. You may reproduce and distribute copies of the
      Work or Derivative Works thereof in any medium, with or without
      modifications, and in Source or Object form, provided that You
      meet the following conditions:

      (a) You must give any other recipients of the Work or
          Derivative Works a copy of this License; and

      (b) You must cause any modified files to carry prominent notices
          stating that You changed the files; and

      (c) You must retain, in the Source form of any Derivative Works
          that You distribute, all copyright, patent, trademark, and
          attribution notices from the Source form of the Work,
          excluding those notices that do not pertain to any part of
          the Derivative Works; and

      (d) If the Work includes a "NOTICE" text file as part of its
          distribution, then any Derivative Works that You distribute must
          include a readable copy of the attribution notices contained
          within such NOTICE file, excluding those notices that do not
          pertain to any part of the Derivative Works, in at least one
          of the following places: within a NOTICE text file distributed
          as part of the Derivative Works; within the Source form or
          documentation, if provided along with the Derivative Works; or,
          within a display generated by the Derivative Works, if and
          wherever such third-party notices normally appear. The contents
          of the NOTICE file are for informational purposes only and
          do not modify the License. You may add Your own attribution
          notices within Derivative Works that You distribute, alongside
          or as an addendum to the NOTICE text from the Work, provided
          that such additional attribution notices cannot be construed
          as modifying the License.

      You may add Your own copyright statement to Your modifications and
      may provide additional or different license terms and conditions
      for use, reproduction, or distribution of Your modifications, or
      for any such Derivative Works as a whole, provided Your use,
      reproduction, and distribution of the Work otherwise complies with
      the conditions stated in this License.

   5. Submission of Contributions. Unless You explicitly state otherwise,
      any Contribution intentionally submitted for inclusion in the Work
      by You to the Licensor shall be under the terms and conditions of
      this License, without any additional terms or conditions.
      Notwithstanding the above, nothing herein shall supersede or modify
      the terms of any separate license agreement you may have executed
      with Licensor regarding such Contributions.

   6. Trademarks. This License does not grant permission to use the trade
      names, trademarks, service marks, or product names of the Licensor,
      except as required for reasonable and customary use in describing the
      origin of the Work and reproducing the content of the NOTICE file.

   7. Disclaimer of Warranty. Unless required by applicable law or
      agreed to in writing, Licensor provides the Work (and each
      Contributor provides its Contributions) on an "AS IS" BASIS,
      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or
      implied, including, without limitation, any warranties or conditions
      of TITLE, NON-INFRINGEMENT, MERCHANTABILITY, or FITNESS FOR A
      PARTICULAR PURPOSE. You are solely responsible for determining the
      appropriateness of using or redistributing the Work and assume any
      risks associated with Your exercise of permissions under this License.

   8. Limitation of Liability. In no event and under no legal theory,
      whether in tort (including negligence), contract, or otherwise,
      unless required by applicable law (such as deliberate and grossly
      negligent acts) or agreed to in writing, shall any Contributor be
      liable to You for damages, including any direct, indirect, special,
      incidental, or consequential damages of any character arising as a
      result of this License or out of the use or inability to use the
      Work (including but not limited to damages for loss of goodwill,
      work stoppage, computer failure or malfunction, or any and all
      other commercial damages or losses), even if such Contributor
      has been advised of the possibility of such damages.

   9. Accepting Warranty or Additional Liability. While redistributing
      the Work or Derivative Works thereof, You may choose to offer,
      and charge a fee for, acceptance of support, warranty, indemnity,
      or other liability obligations and/or rights consistent with this
      License. However, in accepting such obligations, You may act only
      on Your own behalf and on Your sole responsibility, not on behalf
      of any other Contributor, and only if You agree to indemnify,
      defend, and hold each Contributor harmless for any liability
      incurred by, or claims asserted against, such Contributor by reason
      of your accepting any such warranty or additional liability.

   END OF TERMS AND CONDITIONS

   APPENDIX: How to apply the Apache License to your work.

      To apply the Apache License to your work, attach the following
      boilerplate notice, with the fields enclosed by brackets "[]"
      replaced with your own identifying information. (Don't include
      the brackets!)  The text should be enclosed in the appropriate
      comment syntax for the file format. We also recommend that a
      file or class name and description of purpose be included on the
      same "printed page" as the copyright notice for easier
      identification within third-party archives.

   Copyright [2019] [Rolando Gopez Lacuata]

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       https://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


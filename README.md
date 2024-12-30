# OpenH264

This is [OpenH264](https://github.com/cisco/openh264), packaged for
[Zig](https://ziglang.org/).

## Installation

First, update your build.zig.zon:

```bash
zig fetch --save git+https://github.com/gerwin3/openh264.git
```

Next, add this snippet to your build.zig script:

```zig
const openh264_dep = b.dependency("openh264", .{
    .target = target,
    .optimize = optimize,
});
your_compilation.linkLibrary(openh264_dep.artifact("openh264"));
```

This will provide OpenH264 as a static library to `your_compilation`.

TODO: About openh264 module and openh264_bindings module.

## Contribute

This repo is a stripped down version of the original OpenH264 repository:

* The console application has been removed.
* All testing code has been removed. Testing is not part of this package.
* All build stuff has been replaced with Zig.

In practice this means that only the `codec` directory remains, since it
contains the sources.

It should work on most platforms and architectures, but has only been tested on
Linux x86_64. If anything is broken or missing please inspect `build.zig` and
update accordingly. Use the original Meson build files as reference.
Specifically, you may want to update the `BuildConfiguration` for your platform
and architecture to ensure correct compilation of assembly code.

## License

Licensed under either of

 * Apache License, Version 2.0
   ([LICENSE-APACHE](LICENSE-APACHE) or http://www.apache.org/licenses/LICENSE-2.0)
 * MIT license
   ([LICENSE-MIT](LICENSE-MIT) or http://opensource.org/licenses/MIT)

at your option.

Unless you explicitly state otherwise, any contribution intentionally submitted
for inclusion in the work by you, as defined in the Apache-2.0 license, shall be
dual licensed as above, without any additional terms or conditions.

## Credits

This project is made possible by:

* [Cisco's OpenH264 library](https://github.com/cisco/openh264) licensed under
  the BSD 2-Clause License. Please find the [original license](./codec/LICENSE)
  in the `codec` directory.

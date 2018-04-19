#!/usr/bin/env python3

import sys
import os

destdir_prefix = os.environ['MESON_INSTALL_DESTDIR_PREFIX']
libdir = os.path.join(destdir_prefix, sys.argv[1])
suffix = sys.argv[2]
major_version = sys.argv[3]

try:
    os.symlink('libopenh264.{}'.format(suffix), os.path.join(libdir, 'libopenh264.so.{}'.format(major_version)))
except:
    pass

try:
    os.symlink('libopenh264.so.{}'.format(major_version), os.path.join(libdir, 'libopenh264.so'))
except:
    pass


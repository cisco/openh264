#!/usr/bin/env python3

import sys
import os

try:
    os.symlink(sys.argv[1], sys.argv[2])
except:
    pass

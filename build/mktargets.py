#!/usr/bin/python
import sys
import argparse
import os

parser = argparse.ArgumentParser(description="Make helper parser")
parser.add_argument("--directory", dest="directory", required=True)
parser.add_argument("--library", dest="library", help="Make a library")
parser.add_argument("--binary", dest="binary", help="Make a binary")
parser.add_argument("--prefix", dest="prefix", help="Make a set of objs")
parser.add_argument("--exclude", dest="exclude", help="Exclude file", action="append")
parser.add_argument("--include", dest="include", help="Include file", action="append")
parser.add_argument("--out", dest="out", help="Output file")
parser.add_argument("--cpp-suffix", dest="cpp_suffix", help="C++ file suffix")
PREFIX=None
LIBRARY=None
BINARY=None
EXCLUDE=[]
INCLUDE=[]
OUTFILE="targets.mk"
CPP_SUFFIX=".cpp"

def make_o(x):
    return os.path.splitext(x)[0] + ".$(OBJ)"

def write_cpp_rule_pattern(f):
    src = "$(%s_SRCDIR)/%%%s"%(PREFIX, CPP_SUFFIX)
    dst = "$(%s_SRCDIR)/%%.$(OBJ)"%(PREFIX)

    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(QUIET_CXX)$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(' + PREFIX + '_CFLAGS) $(' + PREFIX + '_INCLUDES) -c $(CXX_O) $<\n')
    f.write("\n")

def write_c_rule_pattern(f):
    src = "$(%s_SRCDIR)/%%.c"%(PREFIX)
    dst = "$(%s_SRCDIR)/%%.$(OBJ)"%(PREFIX)

    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(QUIET_CC)$(CC) $(CFLAGS) $(INCLUDES) $(' + PREFIX + '_CFLAGS) $(' + PREFIX + '_INCLUDES) -c $(CXX_O) $<\n')
    f.write("\n")

def write_asm_rule_pattern(f):
    src = "$(%s_SRCDIR)/%%.asm"%(PREFIX)
    dst = "$(%s_SRCDIR)/%%.$(OBJ)"%(PREFIX)

    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(QUIET_ASM)$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(' + PREFIX + '_ASMFLAGS) $(' + PREFIX + '_ASM_INCLUDES) -o $@ $<\n')
    f.write("\n")

def write_asm_s_rule_pattern(f):
    src = "$(%s_SRCDIR)/%%.S"%(PREFIX)
    dst = "$(%s_SRCDIR)/%%.$(OBJ)"%(PREFIX)

    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(QUIET_CCAS)$(CCAS) $(CFLAGS) $(ASMFLAGS) $(INCLUDES) $(' + PREFIX + '_CFLAGS) $(' + PREFIX + '_INCLUDES) -c -o $@ $<\n')
    f.write("\n")


def find_sources():
    cpp_files = []
    asm_files = []
    c_files = []
    s_files = []
    print EXCLUDE
    for dir in os.walk("."):
        for file in dir[2]:
            if (len(INCLUDE) == 0 and not file in EXCLUDE) or file in INCLUDE:
                if os.path.splitext(file)[1] == CPP_SUFFIX:
                    cpp_files.append(os.path.join(dir[0].strip('./'), file))
                if os.path.splitext(file)[1] == '.asm':
                    asm_files.append(os.path.join(dir[0].strip('./'), file))
                if os.path.splitext(file)[1] == '.c':
                    c_files.append(os.path.join(dir[0].strip('./'), file))
                if os.path.splitext(file)[1] == '.S':
                    s_files.append(os.path.join(dir[0].strip('./'), file))
    return [cpp_files, asm_files, c_files, s_files]


args = parser.parse_args()

if args.library is not None:
    PREFIX=args.library.upper()
elif args.binary is not None:
    PREFIX=args.binary.upper()
elif args.prefix is not None:
    PREFIX=args.prefix.upper()
else:
    sys.stderr.write("Must provide either library, binary or prefix")
    sys.exit(1)

if args.exclude is not None:
    EXCLUDE = args.exclude
if args.include is not None:
    INCLUDE = args.include
if args.out is not None:
    OUTFILE = args.out
else:
    OUTFILE = os.path.join(args.directory, OUTFILE)
if args.cpp_suffix is not None:
    CPP_SUFFIX = args.cpp_suffix

OUTFILE = os.path.abspath(OUTFILE)
try:
    os.chdir(args.directory)
except OSError as e:
    sys.stderr.write("Error changing directory to %s\n" % e.filename)
    sys.exit(1)

(cpp, asm, cfiles, sfiles) = find_sources()

cpp = sorted(cpp, key=lambda s: s.lower())
asm = sorted(asm, key=lambda s: s.lower())
cfiles = sorted(cfiles, key=lambda s: s.lower())
sfiles = sorted(sfiles, key=lambda s: s.lower())



f = open(OUTFILE, "w")
f.write("%s_SRCDIR=%s\n"%(PREFIX, args.directory))

f.write("%s_CPP_SRCS=\\\n"%(PREFIX))
for c in cpp:
    f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
f.write("\n")
f.write("%s_OBJS += $(%s_CPP_SRCS:%s=.$(OBJ))\n\n"%(PREFIX, PREFIX, CPP_SUFFIX))

if len(cfiles) > 0:
    f.write("%s_C_SRCS=\\\n"%(PREFIX))
    for cfile in cfiles:
        f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, cfile))
    f.write("\n")
    f.write("%s_OBJS += $(%s_C_SRCS:.c=.$(OBJ))\n\n"%(PREFIX, PREFIX))

if len(asm) > 0:
    f.write("ifeq ($(ASM_ARCH), x86)\n")
    f.write("%s_ASM_SRCS=\\\n"%(PREFIX))
    for c in asm:
        f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
    f.write("\n")
    f.write("%s_OBJS += $(%s_ASM_SRCS:.asm=.$(OBJ))\n"%(PREFIX, PREFIX))
    f.write("endif\n\n")

if len(sfiles) > 0:
    f.write("ifeq ($(ASM_ARCH), arm)\n")
    f.write("%s_ASM_S_SRCS=\\\n"%(PREFIX))
    for c in sfiles:
        f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
    f.write("\n")
    f.write("%s_OBJS += $(%s_ASM_S_SRCS:.S=.$(OBJ))\n"%(PREFIX, PREFIX))
    f.write("endif\n\n")

f.write("OBJS += $(%s_OBJS)\n"%PREFIX)

write_cpp_rule_pattern(f)

if len(cfiles) > 0:
    write_c_rule_pattern(f)

if len(asm) > 0:
    write_asm_rule_pattern(f)

if len(sfiles) > 0:
    write_asm_s_rule_pattern(f)

if args.library is not None:
    f.write("$(LIBPREFIX)%s.$(LIBSUFFIX): $(%s_OBJS)\n"%(args.library, PREFIX))
    f.write("\t$(QUIET)rm -f $@\n")
    f.write("\t$(QUIET_AR)$(AR) $(AR_OPTS) $+\n")
    f.write("\n")
    f.write("libraries: $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library)
    f.write("LIBRARIES += $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library)

if args.binary is not None:
    f.write("%s$(EXEEXT): $(%s_OBJS) $(%s_DEPS)\n"%(args.binary, PREFIX, PREFIX))
    f.write("\t$(QUIET_CXX)$(CXX) $(CXX_LINK_O) $(%s_OBJS) $(%s_LDFLAGS) $(LDFLAGS)\n\n"%(PREFIX, PREFIX))
    f.write("binaries: %s$(EXEEXT)\n"%args.binary)
    f.write("BINARIES += %s$(EXEEXT)\n"%args.binary)

f.close()

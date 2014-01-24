#!/usr/bin/python
import sys
import argparse
import os

parser = argparse.ArgumentParser(description="Make helper parser")
parser.add_argument("--directory", dest="directory", required=True)
parser.add_argument("--library", dest="library", help="Make a library")
parser.add_argument("--binary", dest="binary", help="Make a binary")
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
    return os.path.splitext(x)[0] + ".o"

def write_cpp_rule_pattern(f):
    src = "$(%s_SRCDIR)/%%%s"%(PREFIX, CPP_SUFFIX)
    dst = "$(%s_SRCDIR)/%%.o"%(PREFIX)

    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(' + PREFIX + '_CFLAGS) $(' + PREFIX + '_INCLUDES) -c $(CXX_O) $<\n')
    f.write("\n")

def write_asm_rule_pattern(f):
    src = "$(%s_SRCDIR)/%%.asm"%(PREFIX)
    dst = "$(%s_SRCDIR)/%%.o"%(PREFIX)

    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(ASM) $(ASMFLAGS) $(ASM_INCLUDES) $(' + PREFIX + '_ASMFLAGS) $(' + PREFIX + '_ASM_INCLUDES) -o $@ $<\n')
    f.write("\n")


def find_sources():
    cpp_files = []
    asm_files = []
    print EXCLUDE
    for dir in os.walk("."):
        for file in dir[2]:
            if (len(INCLUDE) == 0 and not file in EXCLUDE) or file in INCLUDE:
                if os.path.splitext(file)[1] == CPP_SUFFIX:
                    cpp_files.append(os.path.join(dir[0], file))
                if os.path.splitext(file)[1] == '.asm':
                    asm_files.append(os.path.join(dir[0], file))
    return [cpp_files, asm_files]


args = parser.parse_args()

if args.library is not None:
    PREFIX=args.library.upper()
elif args.binary is not None:
    PREFIX=args.binary.upper()
else:
    sys.stderr.write("Must provide either library or binary")
    sys.exit(1)

if args.exclude is not None:
    EXCLUDE = args.exclude
if args.include is not None:
    INCLUDE = args.include
if args.out is not None:
    OUTFILE = args.out
if args.cpp_suffix is not None:
    CPP_SUFFIX = args.cpp_suffix
(cpp, asm) = find_sources()



f = open(OUTFILE, "w")
f.write("%s_SRCDIR=%s\n"%(PREFIX, args.directory))

f.write("%s_CPP_SRCS=\\\n"%(PREFIX))
for c in cpp:
    f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
f.write("\n")
f.write("%s_OBJS += $(%s_CPP_SRCS:%s=.o)\n"%(PREFIX, PREFIX, CPP_SUFFIX))

if len(asm) > 0:
    f.write("ifeq ($(USE_ASM), Yes)\n")
    f.write("%s_ASM_SRCS=\\\n"%(PREFIX))
    for c in asm:
        f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
    f.write("\n")
    f.write("%s_OBJS += $(%s_ASM_SRCS:.asm=.o)\n"%(PREFIX, PREFIX))
    f.write("endif\n\n")

f.write("OBJS += $(%s_OBJS)\n"%PREFIX)

write_cpp_rule_pattern(f)

if len(asm) > 0:
    write_asm_rule_pattern(f)

if args.library is not None:
    f.write("$(LIBPREFIX)%s.$(LIBSUFFIX): $(%s_OBJS)\n"%(args.library, PREFIX))
    f.write("\trm -f $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library)
    f.write("\t$(AR) $(AR_OPTS) $(%s_OBJS)\n"%PREFIX)
    f.write("\n")
    f.write("libraries: $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library)
    f.write("LIBRARIES += $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library)

if args.binary is not None:
    f.write("%s$(EXEEXT): $(%s_OBJS) $(LIBS) $(%s_LIBS) $(%s_DEPS)\n"%(args.binary, PREFIX, PREFIX, PREFIX))
    f.write("\t$(CXX) $(CXX_LINK_O)  $(%s_OBJS) $(%s_LDFLAGS) $(%s_LIBS) $(LDFLAGS) $(LIBS)\n\n"%(PREFIX, PREFIX, PREFIX))
    f.write("binaries: %s$(EXEEXT)\n"%args.binary)
    f.write("BINARIES += %s$(EXEEXT)\n"%args.binary)

f.close()

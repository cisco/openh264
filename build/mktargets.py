#!/usr/bin/python
import sys
import argparse
import os

parser = argparse.ArgumentParser(description="Make helper parser")
parser.add_argument("--directory", dest="directory", required=True)
parser.add_argument("--library", dest="library", help="Make a library")
parser.add_argument("--binary", dest="binary", help="Make a binary")
parser.add_argument("--exclude", dest="exclude", help="Exclude file", action="append")
PREFIX=None
LIBRARY=None
BINARY=None
EXCLUDE=[]

def make_o(x):
    return os.path.splitext(x)[0] + ".o"

def write_cpp_rule(f, x):
    src = "$(%s_SRCDIR)/%s"%(PREFIX, x)
    dst = "$(%s_SRCDIR)/%s"%(PREFIX, make_o(x))
    
    f.write("%s: %s\n"%(dst, src))
    f.write('\t$(CXX) $(CFLAGS) $(CXXFLAGS) $(INCLUDES) $(' + PREFIX + '_CFLAGS) $(' + PREFIX + '_INCLUDES) -c -o ' + dst + ' ' + src + '\n');
    f.write("\n")
    

def find_sources():
    cpp_files = []
    asm_files = []
    print EXCLUDE
    for dir in os.walk("."):
        for file in dir[2]:
            if not file in EXCLUDE:
                if os.path.splitext(file)[1] == '.cpp':
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
(cpp, asm) = find_sources()



f = open("targets.mk", "w")
f.write("%s_PREFIX=%s\n"%(PREFIX, PREFIX))
f.write("%s_SRCDIR=%s\n"%(PREFIX, args.directory))

f.write("%s_CPP_SRCS=\\\n"%(PREFIX))
for c in cpp:
    f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
f.write("\n")    
f.write("%s_OBJS += $(%s_CPP_SRCS:.cpp=.o)\n"%(PREFIX, PREFIX))

f.write("ifdef USE_ASM\n")
f.write("%s_ASM_SRCS=\\\n"%(PREFIX))
for c in asm:
    f.write("\t$(%s_SRCDIR)/%s\\\n"%(PREFIX, c))
f.write("\n")
f.write("%s_OBJS += $(%s_ASM_SRCS:.asm=.o)\n"%(PREFIX, PREFIX))
f.write("endif\n\n")

f.write("OBJS += $(%s_OBJS)\n"%PREFIX)

for c in cpp:
    write_cpp_rule(f, c)

#for a in asm:
#    write_asm_rule(f, a)

if args.library is not None:
    f.write("$(LIBPREFIX)%s.$(LIBSUFFIX): $(%s_OBJS)\n"%(args.library, PREFIX));
    f.write("\trm -f $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library)
    f.write("\tar cr $@ $(%s_OBJS)\n"%PREFIX);
    f.write("\n");
    f.write("libraries: $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library);
    f.write("LIBRARIES += $(LIBPREFIX)%s.$(LIBSUFFIX)\n"%args.library);

if args.binary is not None:
    f.write("%s: $(%s_OBJS) $(LIBS) $(%s_LIBS)\n"%(args.binary, PREFIX, PREFIX))
    f.write("\t$(CXX) -o $@  $(%s_OBJS) $(%s_LDFLAGS) $(%s_LIBS) $(LDFLAGS) $(LIBS)\n\n"%(PREFIX, PREFIX, PREFIX))
    f.write("binaries: %s\n"%args.binary);
    f.write("BINARIES += %s\n"%args.binary);

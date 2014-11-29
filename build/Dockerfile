# This is a docker image with all the tools to build openh264 for linux 

# build the docker image with: sudo docker build -t openh264tools - < Dockerfile 
# get the result with: sudo docker run -t -i -v /tmp/openH264:/build openh264tools /bin/cp libopenh264.so log /build
# the results will be left in /tmp/openH264
# have a look at log file and if the hash match the "Fluffy got" hashes

FROM ubuntu:14.04
MAINTAINER Cullen Jennings <fluffy@cisco.com>
RUN apt-get update
RUN apt-get upgrade -y
RUN apt-get install -y bison flex g++ gcc git libgmp3-dev libmpc-dev libmpfr-dev libz-dev make wget

WORKDIR /tmp
RUN wget http://ftp.gnu.org/gnu/gcc/gcc-4.9.2/gcc-4.9.2.tar.gz
RUN tar xvfz gcc-4.9.2.tar.gz
WORKDIR /tmp/gcc-4.9.2/
RUN mkdir build 
WORKDIR /tmp/gcc-4.9.2/build
RUN ../configure --disable-checking --enable-languages=c,c++ --enable-multiarch --enable-shared --enable-threads=posix  --with-gmp=/usr/local/lib --with-mpc=/usr/lib --with-mpfr=/usr/lib --without-included-gettext --with-system-zlib --with-tune=generic --disable-multilib --disable-nls
RUN make -j 8 
RUN make install 

WORKDIR	 /tmp
RUN wget http://www.nasm.us/pub/nasm/releasebuilds/2.11.06/nasm-2.11.06.tar.gz
RUN tar xvfz nasm-2.11.06.tar.gz 
WORKDIR /tmp/nasm-2.11.06/
RUN ./configure
RUN make
RUN make install 

WORKDIR /tmp
RUN git clone https://github.com/cisco/openh264.git
WORKDIR /tmp/openh264
RUN git checkout v1.1
RUN make ENABLE64BIT=Yes

RUN date > log 
RUN uname -a >> log 
RUN nasm -v >> log 
RUN gcc -v 2>> log
RUN git status -v >> log

RUN openssl dgst -sha1 libopenh264.so >> log
RUN echo "Fluffy Got hash of  - 3b6280fce36111ab9c911453f4ee1fd99ce6f841" >> log







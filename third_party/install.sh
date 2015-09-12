# /bin/bash
# install third party library: 1. ZeroMQ, 2. MPICH2

THIRD_PARTY_DIR=$PWD
echo $TARGET_DIR

ZMQ_DIR=zeromq-4.1.3
MPI_DIR=mpich-3.0.4
# 1. Get ZeroMQ
wget http://download.zeromq.org/zeromq-4.1.3.tar.gz
tar -zxf zeromq-4.1.3.tar.gz

# Build ZeroMQ
# Make sure that libtool, pkg-config, build-essential, autoconf and automake
# are installed.
cd $ZMQ_DIR
./configure --prefix=$THIRD_PARTY_DIR --without-libsodium
make -j4
make install -j4
cd ..
rm -rf $ZMQ_DIR

# Get the C++ Wrapper zmq.hpp
wget https://raw.githubusercontent.com/zeromq/cppzmq/master/zmq.hpp
mv zmq.hpp $THIRD_PARTY_DIR/include

# Get MPICH2
wget http://www.mpich.org/static/downloads/3.0.4/mpich-3.0.4.tar.gz
tar -zxf mpich-3.0.4.tar.gz

# Build MPICH2
cd $MPI_DIR
./configure --prefix=$THIRD_PARTY_DIR --disable-fc --disable-f77
make -j4
make install -j4
cd ..
rm -rf $MPI_DIR

rm *.tar.gz*

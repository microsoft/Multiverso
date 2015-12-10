# /bin/bash
# install third party library: 1. ZeroMQ, 2. MPICH2

THIRD_PARTY_DIR=$PWD
echo $TARGET_DIR

ZMQ_DIR=zeromq-4.1.3
MPI_DIR=mpich-3.0.4
HADOOP_DIR=hadoop-2.6.0
JDK_DIR=jdk1.8.0_65
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

# GET JDK
wget --no-check-certificate --no-cookies --header "Cookie: oraclelicense=accept-securebackup-cookie"  http://download.oracle.com/otn-pub/java/jdk/8u65-b17/jdk-8u65-linux-x64.tar.gz
tar -zxvf jdk-8u65-linux-x64.tar.gz
mv JDK_DIR/ jdk/
cp jdk/jre/lib/amd64/server/* lib/
cp -r jdk/include/* include/

# GET hadoop
wget http://mirrors.cnnic.cn/apache/hadoop/common/hadoop-2.6.0/hadoop-2.6.0.tar.gz
tar -zxvf hadoop-2.6.0.tar.gz
mv HADOOP_DIR/ hadoop/
cp hadoop/lib/native/* lib/
cp hadoop/include/* include/

rm *.tar.gz*

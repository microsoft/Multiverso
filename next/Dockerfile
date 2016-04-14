# Creates hadoop-2.6.0 zeromq-4.1.3 mpich-3.0.4 jdk1.8.0_65 dmtk-ftrl on Ubuntu 14.04
#
# docker build -t lyysdy/dmtk_ftrl .

FROM ubuntu:14.04
MAINTAINER lyysdy@foxmail.com

USER root

# install dev tools
RUN apt-get update \
    && apt-get install -qqy curl tar g++-4.8 gcc \
	   libtool pkg-config autoconf openssh-server openssh-client rsync build-essential automake \
       vim

# passwordless ssh
RUN echo "syntax enable\nset nu\nset hlsearch\nset showmatch\nset shiftwidth=4\nset mouse=a\nset tabstop=4" >> ~/.vimrc \
    && rm -f /etc/ssh/ssh_host_dsa_key /etc/ssh/ssh_host_rsa_key /root/.ssh/id_rsa \
    && ssh-keygen -q -N "" -t dsa -f /etc/ssh/ssh_host_dsa_key \
    && ssh-keygen -q -N "" -t rsa -f /etc/ssh/ssh_host_rsa_key \
    && ssh-keygen -q -N "" -t rsa -f /root/.ssh/id_rsa \
    && cp /root/.ssh/id_rsa.pub /root/.ssh/authorized_keys

# Cmake
RUN curl -sc 0 https://cmake.org/files/v3.4/cmake-3.4.1-Linux-x86_64.tar.gz | tar -xz -C /usr/local  \
	&& cd /usr/local \
    && ln -s ./cmake-3.4.1-Linux-x86_64 cmake
ENV PATH $PATH:/usr/local/cmake/bin

# Build ZeroMQ
# Make sure that libtool, pkg-config, build-essential, autoconf and automake
# are installed.
RUN wget -qc -t 0 http://download.zeromq.org/zeromq-4.1.3.tar.gz \
	&& tar -zxvf zeromq-4.1.3.tar.gz \
	&& rm zeromq-4.1.3.tar.gz \
	&& cd zeromq-4.1.3 \
	&& ./configure --prefix=/usr/local --without-libsodium \
    && make -j4 \
	&& make install -j4 \
	&& cd .. \
	&& rm -rf zeromq-4.1.3 

# Get the C++ Wrapper zmq.hpp
RUN wget https://raw.githubusercontent.com/zeromq/cppzmq/master/zmq.hpp \
	&& mv zmq.hpp /usr/local/include 

#
# Get MPICH2
RUN curl -s http://www.mpich.org/static/downloads/3.0.4/mpich-3.0.4.tar.gz | tar -zx \
	&& cd mpich-3.0.4 \
	&& ./configure --prefix=/usr/local --disable-fc --disable-f77 \
	&& make -j4 \
	&& make install -j4 \
	&& cd .. \
	&& rm -rf mpich-3.0.4 

# java
RUN mkdir -p /usr/local/java/default && \
    curl -Ls 'http://download.oracle.com/otn-pub/java/jdk/8u65-b17/jdk-8u65-linux-x64.tar.gz' -H 'Cookie: oraclelicense=accept-securebackup-cookie' | \
    tar --strip-components=1 -xz -C /usr/local/java/default/

ENV JAVA_HOME /usr/local/java/default/ 
ENV PATH $PATH:$JAVA_HOME/bin

# hadoop
RUN wget -cq -t 0 http://www.eu.apache.org/dist/hadoop/common/hadoop-2.6.0/hadoop-2.6.0.tar.gz 
RUN tar -xz -C /usr/local/ -f hadoop-2.6.0.tar.gz \
	&& rm hadoop-2.6.0.tar.gz \
    && cd /usr/local && ln -s ./hadoop-2.6.0 hadoop \
    && cp -r /usr/local/hadoop/include/* /usr/local/include

ENV HADOOP_PREFIX /usr/local/hadoop
RUN sed -i '/^export JAVA_HOME/ s:.*:export JAVA_HOME=/usr/local/java/default\nexport HADOOP_PREFIX=/usr/local/hadoop\nexport HADOOP_HOME=/usr/local/hadoop\n:' $HADOOP_PREFIX/etc/hadoop/hadoop-env.sh
RUN sed -i '/^export HADOOP_CONF_DIR/ s:.*:export HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop/:' $HADOOP_PREFIX/etc/hadoop/hadoop-env.sh

# fixing the libhadoop.so like a boss
RUN rm  /usr/local/hadoop/lib/native/* \
    && curl -Ls http://dl.bintray.com/sequenceiq/sequenceiq-bin/hadoop-native-64-2.6.0.tar | tar -x -C /usr/local/hadoop/lib/native/

ADD docker-ubuntu/core-site.xml.template $HADOOP_PREFIX/etc/hadoop/core-site.xml.template
RUN sed s/HOSTNAME/master.dango.work/ /usr/local/hadoop/etc/hadoop/core-site.xml.template > /usr/local/hadoop/etc/hadoop/core-site.xml

 
ADD docker-ubuntu/ssh_config /root/.ssh/config
RUN chmod 600 /root/.ssh/config \
    && chown root:root /root/.ssh/config

# prepare boostrap.sh
ADD docker-ubuntu/bootstrap.sh /etc/bootstrap.sh
RUN chown root:root /etc/bootstrap.sh \
    && chmod 700 /etc/bootstrap.sh
ENV BOOTSTRAP /etc/bootstrap.sh

# workingaround docker.io build error
RUN ls -la /usr/local/hadoop/etc/hadoop/*-env.sh \
    && chmod +x /usr/local/hadoop/etc/hadoop/*-env.sh \
    && ls -la /usr/local/hadoop/etc/hadoop/*-env.sh

# fix the 254 error code
RUN sed  -i "/^[^#]*UsePAM/ s/.*/#&/"  /etc/ssh/sshd_config \
   && echo "UsePAM no" >> /etc/ssh/sshd_config \
   && echo "Port 2122" >> /etc/ssh/sshd_config

RUN apt-get update
RUN apt-get install -qqy gdb
# compile multiverso
ADD . /project
RUN mkdir -p /project/build \
	&& cd /project/build \
	&& cmake -DUSE_HDFS=OFF .. \
	&& make 
    
# clean unnessary packages
RUN apt-get clean \
	&& rm -rf /var/lib/apt/lists/*

CMD ["/etc/bootstrap.sh", "ls"]

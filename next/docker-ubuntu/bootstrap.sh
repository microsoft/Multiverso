#!/bin/bash

: ${HADOOP_PREFIX:=/usr/local/hadoop}

$HADOOP_PREFIX/etc/hadoop/hadoop-env.sh


# installing libraries if any - (resource urls added comma separated to the ACP system variable)
cd $HADOOP_PREFIX/share/hadoop/common ; for cp in ${ACP//,/ }; do  echo == $cp; curl -LO $cp ; done;

export PATH=$PATH:${HADOOP_PREFIX}/bin
export CLASSPATH=/usr/local/hadoop/lib/native/*:`hadoop classpath --glob`:/usr/local/java/default/lib/*.jar

#/project/build/bin/multiverso.tests

cd /workdir
ls
whoami
$1
exit

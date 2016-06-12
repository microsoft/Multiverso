from setuptools import setup
import shutil
import os
import platform

from distutils.command.build import build
from setuptools.command.develop import develop
from setuptools.command.install_lib import install_lib
from setuptools.command.easy_install import easy_install


def readme():
    with open('README.md') as f:
        return f.read()


setup(name='multiverso-python',
      version='0.1',
      long_description=readme(),
      description="Multiverso is a parameter server framework for distributed"
      " machine learning. This package can leverage multiple machines and GPUs"
      " to speed up the python programs.",
      url='https://github.com/Microsoft/multiverso',
      author='Microsoft',
      license='MIT',
      packages=['multiverso', 'multiverso.theano_ext', 'multiverso.theano_ext.lasagne_ext'],
      install_requires=["theano", "lasagne"],
      zip_safe=False)

from setuptools import setup
import os


def readme():
    with open('README.md') as f:
        return f.read()


setup(name='multiverso-python',
      version='0.0.1',
      long_description=readme(),
      description="Multiverso is a parameter server framework for distributed"
      " machine learning. This package can leverage multiple machines and GPUs"
      " to speed up the python programs.",
      url='https://github.com/Microsoft/multiverso',
      author='Microsoft',
      license='MIT',
      packages=['multiverso', 'multiverso.theano_ext', 'multiverso.theano_ext.lasagne_ext'],
      install_requires=["theano", "lasagne"],
      classifiers=[
        "Intended Audience :: Developers",
        "Intended Audience :: Science/Research",
        "Programming Language :: Python :: 2",
      ],
      zip_safe=False)

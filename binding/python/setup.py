from setuptools import setup
import shutil
import os
import platform

from distutils.command.build import build
from setuptools.command.develop import develop
from setuptools.command.install_lib import install_lib
from setuptools.command.easy_install import easy_install

PACKAGE_PATH = os.path.abspath(os.path.dirname(__file__))
PROJECT_PATH = os.path.abspath(os.path.join(PACKAGE_PATH, os.path.pardir, os.path.pardir))


def copy_dynamic_lib():
    '''
    This function will find the dynamic library and place it to the right place.
    If the dynamic library is found, then it will copy the dynamic library and
    return True. Otherwise it will warn users and return False.
    '''
    # TODO: find better way to get the dynamic library
    if platform.system() == "Windows":
        mv_lib_path = os.path.join(PROJECT_PATH, "src", "x64", "release", "Multiverso.dll")
    else:
        mv_lib_path = os.path.join(PROJECT_PATH, "build", "src", "libmultiverso.so")
    if os.path.exists(mv_lib_path):
        shutil.copy(mv_lib_path, os.path.join(PACKAGE_PATH, "multiverso"))
        return True
    else:
        msg = "The multiverso dynamic library(" + mv_lib_path + ") can't be "\
              "found  , please make sure you have followed the guide here"\
              "(https://github.com/Microsoft/multiverso/#build) and built"\
              "it successfully."
        # Make the message colorful
        print "\033[93m" + msg + '\033[0m'
        return False


# These customized commands will place the multiverso dynamic library to the right place
class mv_build(build):
    def run(self):
        if copy_dynamic_lib():
            build.run(self)


class mv_easy_install(easy_install):
    def run(self):
        if copy_dynamic_lib():
            easy_install.run(self)


class mv_install_lib(install_lib):
    def run(self):
        if copy_dynamic_lib():
            install_lib.run(self)


class mv_develop(develop):
    def run(self):
        if copy_dynamic_lib():
            develop.run(self)


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
      cmdclass={'build': mv_build,
                'easy_install': mv_easy_install,
                'install_lib': mv_install_lib,
                'develop': mv_develop},
      package_dir={'multiverso': 'multiverso'},
      package_data={
          'multiverso': ['libmultiverso.so', 'Multiverso.dll'],
      },
      include_package_data=True,
      zip_safe=False)

from setuptools import setup
from setuptools.command.install import install
import shutil
import os
import platform

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
        mv_lib_path = os.path.join(PROJECT_PATH, "x64", "release", "Multiverso.dll")
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


class mv_install(install):
    '''
    This customized command will place the multiverso.so to the right place.
    '''
    def run(self):
        # TODO: find better way to get the libmultiverso.so
        if copy_dynamic_lib():
            install.run(self)


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
      packages=['examples.theano', 'examples.theano.lasagne', 'multiverso',
          'multiverso.theano_ext', 'multiverso.theano_ext.lasagne_ext'],
      install_requires=["theano", "lasagne"],
      cmdclass={"install": mv_install},
      package_dir={'multiverso': 'multiverso'},
      package_data={
          'multiverso': ['libmultiverso.so', 'Multiverso.dll'],
      },
      include_package_data=True,
      zip_safe=False)

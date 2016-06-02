from setuptools import setup
from setuptools.command.install import install
import shutil
import os

PACKAGE_PATH = os.path.abspath(os.path.dirname(__file__))
PROJECT_PATH = os.path.abspath(os.path.join(PACKAGE_PATH, os.path.pardir, os.path.pardir))


class mv_install(install):
    '''
    This customized command will place the multiverso.so to the right place.
    '''
    def run(self):
        # TODO: find better way to get the libmultiverso.so
        mv_so = os.path.join(PROJECT_PATH, "build", "src", "libmultiverso.so")
        if os.path.exists(mv_so):
            shutil.copy(mv_so, os.path.join(PACKAGE_PATH, "multiverso"))
            install.run(self)
        else:
            msg = "The libmultiverso.so(" + mv_so + ") can't be found, please"\
                  " make sure you have followed the guide here"\
                  "(https://github.com/Microsoft/multiverso/#build) and built"\
                  "it successfully."
            print "\033[93m" + msg + '\033[0m'


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
          'multiverso': ['libmultiverso.so'],
      },
      include_package_data=True,
      zip_safe=False)

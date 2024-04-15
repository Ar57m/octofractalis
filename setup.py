from setuptools import setup
from Cython.Build import cythonize
import numpy

setup(
    ext_modules = cythonize("fractal_generator.pyx", compiler_directives={'language_level': "3str", 'boundscheck': False}),
    include_dirs=[numpy.get_include()]
) 
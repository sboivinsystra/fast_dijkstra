from setuptools import setup, Extension
import pybind11
import os

boost_include = os.environ.get("BOOST_INCLUDEDIR", "")
ext_modules = [
    Extension(
        "pyboostgraph",
        ["pyboostgraph.cpp"],
        include_dirs=[pybind11.get_include(), boost_include],
        language="c++",
        extra_compile_args=["-O3", "-fopenmp"],  # Enable OpenMP
        extra_link_args=["-fopenmp"],  # Link OpenMP library
    ),
]

setup(
    name="pyboostgraph",
    version="0.14",
    ext_modules=ext_modules,
)

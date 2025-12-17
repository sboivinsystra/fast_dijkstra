from setuptools import setup, Extension
import pybind11
import platform
import os

system = platform.system()
boost_include = os.environ.get("BOOST_INCLUDEDIR", "")


include_dirs = [pybind11.get_include()]
if system == "Windows":
    print("boost_include: ", boost_include)
    include_dirs.append(boost_include)


# Define OpenMP flags based on the compiler
def get_openmp_flags():
    if system == "Windows":
        return ["/openmp"], []  # Compile flags, Link flags
    elif system == "Darwin":  # macOS (Apple Clang) is tricky with OpenMP
        # You might need 'libomp' installed via brew
        return ["-Xpreprocessor", "-fopenmp"], ["-lomp"]
    else:  # Linux / GCC
        return ["-fopenmp"], ["-fopenmp"]


omp_compile_args, omp_link_args = get_openmp_flags()

ext_modules = [
    Extension(
        "boostpy",
        ["boostpy.cpp"],
        include_dirs=include_dirs,
        language="c++",
        extra_compile_args=omp_compile_args,
        extra_link_args=omp_link_args,
    ),
]

setup(
    name="boostpy",
    version="1.0",
    ext_modules=ext_modules,
)

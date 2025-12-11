from setuptools import setup, Extension
import pybind11
import platform
import os

boost_include = os.environ.get("BOOST_INCLUDEDIR", "")
print("boost_include: ", boost_include)


# Define OpenMP flags based on the compiler
def get_openmp_flags():
    system = platform.system()
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
        "pyboostgraph",
        ["pyboostgraph.cpp"],
        include_dirs=[pybind11.get_include(), boost_include],
        language="c++",
        extra_compile_args=omp_compile_args,
        extra_link_args=omp_link_args,
    ),
]

setup(
    name="pyboostgraph",
    version="0.14",
    ext_modules=ext_modules,
)

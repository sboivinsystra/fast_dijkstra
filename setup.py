from setuptools import setup, Extension
import pybind11
import platform

system = platform.system()


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
        "fast_dijkstra",
        sources=["dijkstra.cpp"],
        include_dirs=[pybind11.get_include()],
        language="c++",
        extra_compile_args=omp_compile_args,
        extra_link_args=omp_link_args,
    ),
]

setup(
    ext_modules=ext_modules,
)

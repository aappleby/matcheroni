import os
import sys
import glob

sys.path.append("symlinks/hancho")
import hancho

cwd_path  = os.getcwd()
self_path = __file__
rel_path  = os.path.relpath(self_path, cwd_path)
rel_dir   = os.path.split(rel_path)[0]
self_file = os.path.split(rel_path)[1]

print(cwd_path)
print(self_path)
print(rel_path)
print(rel_dir)
print(self_file)

src_file = "regex_benchmark.cpp"
obj_file = os.path.splitext(src_file)[0] + ".o"

obj_dir  = os.path.join("obj", rel_dir)
obj_path = os.path.join(obj_dir, obj_file)

build_dir  = os.path.join("build", rel_dir)
build_path = os.path.join(build_dir, "regex_benchmark")

print(obj_path)
print(bin_path)

print(os.path.join(rel_dir, "*.cpp"))
print(glob.glob(os.path.join(rel_dir, "*.cpp")))
print(glob.glob("*.cpp"))

#files = ["regex_benchmark.cpp", "regex_parser.cpp"]

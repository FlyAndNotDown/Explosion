import os

from conan import ConanFile
from conan.errors import ConanException, ConanInvalidConfiguration
from conan.tools.build import can_run, check_min_cppstd
from conan.tools.cmake import CMake, CMakeDeps, CMakeToolchain, cmake_layout
from conan.tools.files import copy
from conan.tools.scm import Git

required_conan_version = ">=2.0.9"


class SwiftShaderConan(ConanFile):
    name = "swiftshader"
    description = "CPU-based Vulkan implementation"
    license = "Apache-2.0"
    url = "https://github.com/conan-io/conan-center-index"
    homepage = "https://github.com/google/swiftshader"
    topics = ("vulkan", "driver", "software-renderer")
    package_type = "shared-library"
    settings = "os", "arch", "compiler", "build_type"

    def layout(self):
        cmake_layout(self, src_folder="src")

    def validate(self):
        check_min_cppstd(self, 17)
        supported_arches = {
            "Windows": ("x86_64",),
            "Macos": ("armv8", "x86_64"),
            "Linux": ("x86_64", "armv8"),
        }
        if str(self.settings.os) not in supported_arches or str(self.settings.arch) not in supported_arches[str(self.settings.os)]:
            raise ConanInvalidConfiguration(f"SwiftShader is not supported on {self.settings.os}/{self.settings.arch}")

    def build_requirements(self):
        self.tool_requires("ninja/[>=1.12]")
        self.tool_requires("cmake/[>=3.22.1]")

    def source(self):
        git = Git(self)
        commit = self.conan_data["sources"][self.version]["commit"]
        git.run("init .")
        git.run("remote add origin https://github.com/google/swiftshader.git")
        git.run(f"fetch --depth 1 origin {commit}")
        git.run("checkout --detach FETCH_HEAD")

    def generate(self):
        toolchain = CMakeToolchain(self, generator="Ninja")
        toolchain.cache_variables["SWIFTSHADER_BUILD_TESTS"] = False
        toolchain.cache_variables["SWIFTSHADER_BUILD_BENCHMARKS"] = False
        toolchain.cache_variables["SWIFTSHADER_BUILD_PVR"] = False
        toolchain.cache_variables["SWIFTSHADER_WARNINGS_AS_ERRORS"] = False
        if self.settings.os == "Linux":
            toolchain.cache_variables["SWIFTSHADER_BUILD_WSI_XCB"] = False
            toolchain.cache_variables["SWIFTSHADER_BUILD_WSI_WAYLAND"] = False
        toolchain.generate()

        deps = CMakeDeps(self)
        deps.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build(target="vk_swiftshader")

    def package(self):
        copy(self, "LICENSE.txt", self.source_folder, os.path.join(self.package_folder, "licenses"))

        cmake_system_names = {"Macos": "Darwin", "Windows": "Windows", "Linux": "Linux"}
        output_folder = os.path.join(self.build_folder, cmake_system_names[str(self.settings.os)])
        runtime_folder = os.path.join(self.package_folder, "bin" if self.settings.os == "Windows" else "lib")
        copy(self, "vk_swiftshader_icd.json", output_folder, runtime_folder)
        if self.settings.os == "Windows":
            copy(self, "vk_swiftshader.dll", output_folder, runtime_folder)
            copy(self, "vk_swiftshader.pdb", output_folder, runtime_folder)
            library_name = "vk_swiftshader.dll"
        elif self.settings.os == "Macos":
            copy(self, "libvk_swiftshader.dylib", output_folder, runtime_folder)
            library_name = "libvk_swiftshader.dylib"
        else:
            copy(self, "libvk_swiftshader.so", output_folder, runtime_folder)
            library_name = "libvk_swiftshader.so"

        for required_file in ("vk_swiftshader_icd.json", library_name):
            if not os.path.isfile(os.path.join(runtime_folder, required_file)):
                raise ConanException(f"Required SwiftShader runtime file was not packaged: {required_file}")

    def package_info(self):
        runtime_dir = "bin" if self.settings.os == "Windows" else "lib"
        manifest = os.path.join(self.package_folder, runtime_dir, "vk_swiftshader_icd.json")
        self.runenv_info.define_path("VK_DRIVER_FILES", manifest)
        self.cpp_info.bindirs = ["bin"] if self.settings.os == "Windows" else []
        self.cpp_info.includedirs = []
        self.cpp_info.libdirs = []
        self.cpp_info.libs = []

    def test(self):
        if can_run(self):
            bin_path = os.path.join(self.cpp.build.bindirs[0], "test_package")
            self.run(bin_path, env="conanrun")

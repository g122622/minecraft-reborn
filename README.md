构建命令

# 配置环境变量

  $env:VCPKG_ROOT = "D:\tools\vcpkg"

# 配置项目

  cmake -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE=D:/tools/vcpkg/scripts/buildsystems/vcpkg.cmake

# 编译

  cmake --build build --config Debug

# 运行测试

  ./build/bin/Debug/mr_tests.exe

# 运行服务端

  ./build/bin/Debug/minecraft-server.exe --help

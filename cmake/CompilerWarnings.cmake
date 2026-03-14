# CompilerWarnings.cmake
# 设置编译器警告选项

function(mc_set_compiler_warnings target)
    if(MSVC)
        # MSVC警告选项
        target_compile_options(${target} PRIVATE
            /W4                    # 高警告级别
            /WX-                   # 警告不作为错误（可选开启）
            /wd4251                # 禁用：DLL导出类成员警告
            /wd4275                # 禁用：DLL导出基类警告
            /wd4100                # 禁用：未引用的形参警告（使用注释语法 /* param */ 替代）
            /wd4189                # 禁用：未引用的局部变量警告
            /wd4834                # 禁用：放弃 [[nodiscard]] 返回值警告
        )

        # 可选：将警告视为错误
        option(MC_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
        if(MC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE /WX)
        endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        # GCC/Clang警告选项
        target_compile_options(${target} PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow
            -Wconversion
            -Wsign-conversion
            -Wold-style-cast
            -Wnon-virtual-dtor
            -Wunused-parameter      # 警告未使用的参数
            -Wunused-variable       # 警告未使用的变量
        )

        # 可选：将警告视为错误
        option(MC_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
        if(MC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

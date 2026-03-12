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
        )

        # 可选：将警告视为错误
        option(MC_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
        if(MC_WARNINGS_AS_ERRORS)
            target_compile_options(${target} PRIVATE -Werror)
        endif()
    endif()
endfunction()

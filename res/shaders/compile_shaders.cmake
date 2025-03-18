function(compile_shaders)
    set(cur_dir ${PROJECT_SOURCE_DIR}/res/shaders)
    set(bat ${cur_dir}/compile.bat)
    file(READ ${cur_dir}/precompile_shaders.txt precompile_shaders)
    string(REGEX REPLACE ";" "\\\\;" precompile_shaders ${precompile_shaders})
    string(REGEX REPLACE "\n" ";" precompile_shaders ${precompile_shaders})
    set(command_outputs)
    foreach(shader IN LISTS precompile_shaders)
        set(output_spv ${shader})
        set(output_spv ${output_spv}.spv)
        list(APPEND command_outputs ${cur_dir}/${output_spv})
        add_custom_command(
            OUTPUT ${cur_dir}/${output_spv}
            COMMAND cmd /c ${bat} ${cur_dir}/${shader}
            DEPENDS ${cur_dir}/${shader}
        )
    endforeach()
    add_custom_target(
        PreCompileShaders
        DEPENDS ${command_outputs}
    )
    add_dependencies(JRenderApp PreCompileShaders)
endfunction()

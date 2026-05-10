# ─── Plugin Build Rules ──────────────────────────────────────
# Each plugin is a shared library (.so) that exports CreateModule/DestroyModule.
# Plugins link against collect_core headers but NOT the library itself
# (symbols are resolved at runtime via -rdynamic on the main executable).

# Common include path for all plugins
set(PLUGIN_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/src)

# Helper function to define a plugin target
function(add_collect_plugin PLUGIN_NAME)
    set(PLUGIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/${PLUGIN_NAME})

    file(GLOB PLUGIN_SOURCES "${PLUGIN_DIR}/*.cpp" "${PLUGIN_DIR}/*.c")

    if(NOT PLUGIN_SOURCES)
        message(STATUS "Skipping plugin '${PLUGIN_NAME}': no sources found")
        return()
    endif()

    # Output as: lib/plugins/<name>/<name>.so
    add_library(plugin_${PLUGIN_NAME} SHARED ${PLUGIN_SOURCES})

    target_include_directories(plugin_${PLUGIN_NAME}
        PRIVATE
            ${PLUGIN_INCLUDE_DIRS}
            ${PLUGIN_DIR}
    )

    set_target_properties(plugin_${PLUGIN_NAME} PROPERTIES
        OUTPUT_NAME "${PLUGIN_NAME}"
        PREFIX ""                           # no 'lib' prefix
        SUFFIX ".so"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/modules/${PLUGIN_NAME}"
    )

    if(APPLE)
        target_link_options(plugin_${PLUGIN_NAME}
            PRIVATE
                "-undefined"
                "dynamic_lookup"
        )
    endif()
endfunction()

# ─── Register Plugins ───────────────────────────────────────
# Read plugins
add_collect_plugin(cpu)
add_collect_plugin(memory)
add_collect_plugin(df)
add_collect_plugin(uptime)

# Flush-only plugins
add_collect_plugin(network)
add_collect_plugin(thread)
add_collect_plugin(dmesg)

# Write plugins
add_collect_plugin(csv)
add_collect_plugin(logfile)
add_collect_plugin(json_writer)

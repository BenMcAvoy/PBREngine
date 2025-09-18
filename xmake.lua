add_requires("glfw")
add_requires("glad", {configs = {gl = "gl-4.6"}})
add_requires("glm")
add_requires("imgui v1.92.1-docking", {configs = {glfw_opengl3 = true, freetype = true}})
add_requires("spdlog")
add_requires("stb")

add_rules("mode.debug", "mode.release")

set_languages("cxx20")

target("PBREngine")
    set_kind("binary")
    add_files("src/**.cpp")
	add_includedirs("src", {public = true})
	add_packages("glfw", "glad", "glm", "imgui", "spdlog", "stb")

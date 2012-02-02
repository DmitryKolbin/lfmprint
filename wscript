def set_options(opt):
    opt.tool_options("compiler_cxx")

def configure(conf):
    conf.check_tool("compiler_cxx")
    conf.check_tool("node_addon")

    conf.check_cfg(package='libavformat', args='--cflags --libs', uselib_store='libavformat')
    conf.check_cfg(package='libavcodec', args='--cflags --libs', uselib_store='libavcodec')
    conf.check_cfg(package='libavutil', args='--cflags --libs', uselib_store='libavutil')
    conf.check_cfg(package='libsamplerate', args='--cflags --libs', uselib_store='libsamplerate')
    conf.check_cfg(package='fftw3f', args='--cflags --libs', uselib_store='libfftw3f')
    conf.check_cfg(package='liblastfmfp', args='--cflags --libs', uselib_store='liblastfmfp')




def build(bld):
    obj = bld.new_task_gen("cxx", "shlib", "node_addon")
    obj.cxxflags = ["-g", "-D_FILE_OFFSET_BITS=64", "-D__STDC_CONSTANT_MACROS", "-D_LARGEFILE_SOURCE", "-Wall"]
    obj.linkflags = ["-lavcodec", "-lavformat", "-lavutil", "-llastfmfp", "-lfftw3f", "-lsamplerate"]
    obj.target = "lfmprint"
    obj.source = ["src/lfmprintNode.cpp","src/lfmprint.cpp"]
    obj.uselib = ["libavformat", "libavcodec", "libavutil", "liblastfmfp", "libsamplerate", "libfftw3f"]

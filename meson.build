project('fft-client', 'c',
	default_options : [ 'warning_level=3',
                        'buildtype=debugoptimized' ],
	version : '0.1.0',
	license : 'MIT')

cc = meson.get_compiler('c')

c_args = []

m_dep = cc.find_library('m')
fftw_dep = cc.find_library('fftw3')
SDL2_dep = cc.find_library('SDL2')
SDL2_ttf_dep = cc.find_library('SDL2_ttf')

sources = ['main.c']

executable('fft-client',
    sources,
    c_args: c_args,
    dependencies: [m_dep,fftw_dep,SDL2_dep, SDL2_ttf_dep]
)

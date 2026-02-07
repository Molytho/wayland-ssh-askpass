project(
  'wayland-ssh-askpass',
  'cpp',
  version : '1.1',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++20'],
)

dependencies = [
  dependency('gtkmm-4.0'),
  dependency('gtk4-layer-shell-0'),
  dependency('gtk4-x11'),
  dependency('x11'),
]

sources = [
  'src/main.cpp',
  'src/window.cpp',
  'src/window-wayland.cpp',
  'src/window-x11.cpp'
]

includes = include_directories('include')

executable(
  'wayland-ssh-askpass',
  sources,
  include_directories : includes,
  install : true,
  dependencies : dependencies,
)

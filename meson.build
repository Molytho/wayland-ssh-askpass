project(
  'wayland-ssh-askpass',
  'cpp',
  version : '1.0',
  meson_version : '>= 1.3.0',
  default_options : ['warning_level=3', 'cpp_std=c++20'],
)

dependencies = [
  dependency('gtkmm-4.0'),
  dependency('gtk4-layer-shell-0')
]

sources = [
  'src/window.cpp',
  'src/main.cpp'
]

executable(
  'wayland-ssh-askpass',
  sources,
  install : true,
  dependencies : dependencies,
)

project(
    'wayland-askpasses',
    'cpp',
    version : '1.1',
    meson_version : '>= 1.3.0',
    default_options : [
        'warning_level=3',
        'werror=true',
        'cpp_std=c++20'
    ]
)


common_dependencies = [
    dependency('gtkmm-4.0'),
    dependency('gtk4-layer-shell-0'),
    dependency('gtk4-x11'),
    dependency('x11')
]

common_sources = [
    'src/common/window.cpp',
    'src/common/window-wayland.cpp',
    'src/common/window-x11.cpp'
]

common_includes = [
    include_directories('include/common')
]


ssh_askpass_dependencies = common_dependencies

ssh_askpass_sources = common_sources + [
    'src/ssh-askpass/main.cpp',
    'src/ssh-askpass/model.cpp'
]

ssh_askpass_includes = common_includes + [
    include_directories('include/ssh-askpass')
]

executable(
    'wayland-ssh-askpass',
    ssh_askpass_sources,
    include_directories : ssh_askpass_includes,
    install : true,
    dependencies : ssh_askpass_dependencies
)


systemd_askpass_dependencies = common_dependencies + [
    dependency('boost', modules: ['program_options'])
]

systemd_askpass_sources = common_sources + [
    'src/systemd-askpass/main.cpp',
    'src/systemd-askpass/model.cpp',
    'src/systemd-askpass/systemd-askpass-context.cpp'
]

systemd_askpass_includes = common_includes + [
    include_directories('include/systemd-askpass')
]

executable(
    'wayland-systemd-askpass',
    systemd_askpass_sources,
    include_directories : systemd_askpass_includes,
    install : true,
    dependencies : systemd_askpass_dependencies
)
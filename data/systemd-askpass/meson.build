systemd = dependency('systemd')

systemd_config = configuration_data()
systemd_config.set('BIN_DIR', get_option('prefix') / get_option('bindir'))
systemd_user_unit_dir = systemd.get_variable(
    pkgconfig: 'systemduserunitdir',
    pkgconfig_define: ['prefix', get_option('prefix')])


configure_file(input : 'wayland-systemd-askpass.service.in',
               output : 'wayland-systemd-askpass.service',
               configuration : systemd_config,
               install_dir : systemd_user_unit_dir,
               install_tag: 'systemd-askpass')
install_data('wayland-systemd-askpass.path',
             install_dir : systemd_user_unit_dir,
             install_tag: 'systemd-askpass')
test_suites = {
  'rest': [
    'proxy',
    'proxy-continuous',
    'threaded',
    'xml',
    'custom-serialize',
    'oauth2',
    'params',
  ],
  'rest-extras': [
    'flickr',
    'lastfm',
  ],
}

test_deps = [
  glib_dep,
  libsoup_dep,
  librest_dep,
  librest_extras_dep,
  config_dep,
]

test_env = [
  'G_TEST_SRCDIR=@0@'.format(meson.current_source_dir()),
  'G_TEST_BUILDDIR=@0@'.format(meson.current_build_dir()),
]

# Most tests require a D-Bus session, so try to wrap automatically with dbus-run-session
dbus_run_session = find_program('dbus-run-session', required: false)
if dbus_run_session.found()
  add_test_setup('dbus',
    exe_wrapper: dbus_run_session,
    is_default: true,
  )
else
  message('dbus-run-session not found. Please note that tests might fail if you don\'t set up a D-Bus session.')
endif

foreach suite, test_names : test_suites
  foreach name : test_names
    test_bin = executable(name,
      ['@0@.c'.format(name), 'helper/test-server.c'],
      dependencies: test_deps,
    )

    test(name, test_bin,
      suite: suite,
      env: test_env,
    )
  endforeach
endforeach

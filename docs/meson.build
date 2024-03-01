if get_option('gtk_doc') and get_option('introspection')
  dependency('gi-docgen', version: '>= 2021.6',
    native: true,
    fallback: ['gi-docgen', 'dummy_dep'],
  )

  gidocgen = find_program('gi-docgen', required: get_option('gtk_doc'))

  rest_docdir = rest_datadir / 'doc'

  toml_conf = configuration_data()
  toml_conf.set('REST_VERSION', meson.project_version())
  toml_conf.set('SOUP_VERSION', libsoup_api_version)

  rest_toml = configure_file(
    input: 'librest.toml.in',
    output: 'librest.toml',
    configuration: toml_conf,
  )

  custom_target('librest-doc',
    input: librest_gir[0],
    output: 'librest-1.0',
    command: [
      gidocgen,
      'generate',
      '--quiet',
      '--fatal-warnings',
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../rest'),
      '--config', rest_toml,
      '--output-dir=@OUTPUT@',
      '--no-namespace-dir',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT@',
    ],
    depend_files: [ rest_toml ],
    build_by_default: true,
    install: true,
    install_dir: rest_docdir,
  )

  # if get_option('tests')
  #   test('doc-check',
  #     gidocgen,
  #     args: [
  #       'check',
  #       '--config', rest_toml,
  #       '--add-include-path=@0@'.format(meson.current_build_dir() / '../rest'),
  #       librest_gir[0],
  #     ],
  #     depends: librest_gir[0],
  #     suite: ['docs'],
  #   )
  # endif

endif


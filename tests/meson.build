
tests = [
  'custom-serialize',
  'oauth',
  'oauth2',
  'oauth-async',
  'proxy',
  'proxy-continuous',
  'threaded',
  #  'xml'   Broken.
]

foreach test_name : tests
  testcase = executable(
    test_name,
    test_name + '.c',
    'test-server.c',
    dependencies: librest_dep
  )
  test(test_name, testcase)

endforeach


# rest-extras tests

if get_option('extras')
  extras_tests = [
    'flickr',
    'lastfm'
  ]

  foreach test_name : extras_tests
    testcase = executable(
      test_name,
      test_name + '.c',
      dependencies: librest_extras_dep
    )
    test(test_name, testcase)

  endforeach

endif

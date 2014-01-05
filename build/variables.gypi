{
  'target_defaults': {
    'conditions': [
      ['OS=="mac"', {
        'defines': [
          'MACOSX',
        ],
        'include_dirs': [
          '/usr/local/include/',
        ],
        'xcode_settings': {
          'OTHER_CPLUSPLUSFLAGS': [
            '-Wall',
            '-std=c++11',
            '-stdlib=libc++',
          ],
          'OTHER_LDFLAGS': [
            '-L/usr/local/lib',
          ],
        },
      }],
      ['OS=="win"', {
        'defines': [
          '_USE_MATH_DEFINES',
          '_VARIADIC_MAX=8',
          'GLFW_DLL',
        ],
        'include_dirs': [
          '../lib/boost_1_50_0',
        ],
      }],
    ],
  },
}

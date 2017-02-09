{
 'targets': [{
    'target_name': 'turbonet',
    'include_dirs' : [
      "<!(node -e \"require('nan')\")",
    ],
    'sources': [
      'src/socket.cc',
      'src/stream.cc',
      'src/binding.cc',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': [
        '-O3',
      ]
    },
    'cflags': [
      '-O3',
    ],
  }]
}

{
  "targets": [{
    "target_name": "turbo_net",
    "include_dirs": [
      "<!(node -e \"require('napi-macros')\")",
    ],
    "sources": [
      "./src/turbo_net.c",
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

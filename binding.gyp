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
        '-std=c99',
        '-D_GNU_SOURCE'
      ]
    },
    'cflags': [
      '-O3',
      '-std=c99',
      '-D_GNU_SOURCE'
    ],
  }]
}

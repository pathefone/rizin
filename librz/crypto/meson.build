crypto_plugins = {
  'conf_name': 'plugins_crypto',
  'base_cname': 'rz_crypto_plugin_',
  'list': [
    'aes',
    'aes_cbc',
    'base64',
    'base91',
    'blowfish',
    'cps2',
    'des',
    'punycode',
    'rc2',
    'rc4',
    'rc6',
    'rol',
    'ror',
    'rot',
    'serpent',
    'xor',
  ],
}

rz_crypto_sources = [
  'des.c',
  'crypto.c',
  'p/crypto_aes.c',
  'p/crypto_aes_cbc.c',
  'p/crypto_base64.c',
  'p/crypto_base91.c',
  'p/crypto_blowfish.c',
  'p/crypto_cps2.c',
  'p/crypto_des.c',
  'p/crypto_punycode.c',
  'p/crypto_rc2.c',
  'p/crypto_rc4.c',
  'p/crypto_rc6.c',
  'p/crypto_rol.c',
  'p/crypto_ror.c',
  'p/crypto_rot.c',
  'p/crypto_serpent.c',
  'p/crypto_serpent_algo.c',
  'p/crypto_xor.c',
]

nettle_proj = subproject('nettle')
aes_dep = nettle_proj.get_variable('aes_dep')

rz_crypto = library('rz_crypto', rz_crypto_sources,
  include_directories: [platform_inc],
  dependencies: [rz_util_dep, aes_dep],
  install: true,
  implicit_include_directories: false,
  install_rpath: rpath_lib,
  soversion: rizin_libversion,
  version: rizin_version,
  name_suffix: lib_name_suffix,
  name_prefix: lib_name_prefix,
)

rz_crypto_dep = declare_dependency(link_with: rz_crypto,
                                  include_directories: [platform_inc])
meson.override_dependency('rz_crypto', rz_crypto_dep)

modules += { 'rz_crypto': {
    'target': rz_crypto,
    'dependencies': ['rz_util'],
    'plugins': [crypto_plugins]
}}

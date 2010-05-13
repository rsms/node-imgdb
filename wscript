import Options, Utils, re, sys
from os import unlink
from os.path import exists
from shutil import copy2 as copy
srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def _shell(cmd):
  return re.split('[ \t\r\n]+', Utils.cmd_output(cmd).strip())

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.env.append_value('CXXFLAGS', ['-O2', '-g'])
  conf.env.append_value('LDFLAGS', ['-lstdc++'])
  try:
    magickCxxConfig = conf.find_program('Magick++-config',
      var='MAGICKCXX_CONFIG', mandatory=True)
    conf.check_message_1('checking for imagemagick')
    conf.env.append_value('CXXFLAGS', _shell(magickCxxConfig+' --cxxflags --cppflags'))
    conf.env['CXXFLAGS'] = list(set(conf.env['CXXFLAGS']))
    conf.env.append_value('LDFLAGS', _shell(magickCxxConfig+' --ldflags --libs'))
    conf.env['LDFLAGS'] = list(set(conf.env['LDFLAGS']))
    conf.env['LINKFLAGS'] = conf.env['LDFLAGS']
    conf.check_message_2('ok')
  except:
    conf.fatal('imagemagick not found (not installed?)')

def build(bld):
  t = bld.new_task_gen("cxx", "shlib", "node_addon")
  t.target = "binding"
  t.source = ['src/index.cc','src/imgdb.cc','src/haar.cc','src/bloom_filter.cc']
  #t.includes = ["."]
  #t.lib = []

def shutdown():
  # HACK to get binding.node out of build directory.
  if Options.commands['clean']:
    if exists('imgdb/binding.node'): unlink('imgdb/binding.node')
  else:
    if exists('build/default/binding.node'):
      copy('build/default/binding.node', 'imgdb/binding.node')

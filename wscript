import Utils, re, sys
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
    conf.check_message_1('checking for imagemagick')
    # Magick++-config will fail if we are missing imagemagick
    conf.env.append_value('CXXFLAGS', _shell('Magick++-config --cxxflags --cppflags'))
    conf.env['CXXFLAGS'] = list(set(conf.env['CXXFLAGS']))
    conf.env.append_value('LDFLAGS', _shell('Magick++-config --ldflags --libs'))
    conf.env['LDFLAGS'] = list(set(conf.env['LDFLAGS']))
    conf.check_message_2('ok')
  except:
    conf.fatal('imagemagick not found (not installed?)')

def build(bld):
  t = bld.new_task_gen("cxx", "shlib", "node_addon")
  t.target = "imgdb"
  t.source = ['src/index.cc','src/imgdb.cc','src/haar.cc','src/bloom_filter.cc']
  t.includes = ["."]
  t.lib = ["stdc++"]

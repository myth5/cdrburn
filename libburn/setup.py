from distutils.core import setup, Extension

sources = ["src/burnmodule.c", "src/disc.c", "src/drive.c",
           "src/drive_info.c", "src/message.c", "src/progress.c",
           "src/read_opts.c", "src/session.c", "src/source.c",
           "src/toc_entry.c", "src/track.c", "src/write_opts.c"]

include_dirs = ["/usr/include", "/usr/local/include"]
library_dirs = ["/usr/lib", "/usr/local/lib"]
libraries = ["burn", "pthread"]

long_description = \
"""Python Interface to libBurn 0.2.2

pyburn is a stupid binding to libBurn, the awesome burning library"""

module = Extension('burn',
                    define_macros = [('MAJOR_VERSION', '0'),
                                     ('MINOR_VERSION', '1')],
                    include_dirs = include_dirs,
                    libraries = libraries,
                    sources = sources)

setup (name = 'burn',
       version = '0.1',
       description = "Stupid bindings to libBurn 0.2.2",
       long_description=long_description,
       author = "Anant Narayanan",
       author_email = "anant@kix.in",
       license = "GPLv2",
       platforms = "POSIX",
       url = "http://libburn.pykix.org/",
       ext_modules = [module])

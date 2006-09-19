from distutils.core import setup, Extension

module = Extension('burn',
                    define_macros = [('MAJOR_VERSION', '0'),
                                     ('MINOR_VERSION', '1')],
                    include_dirs = ['/usr/local/include'],
                    libraries = ['burn', 'pthread'],
                    sources = ['burnmodule.c'])

setup (name = 'burn',
       version = '0.1',
       description = 'Python Bindings for libburn',
       author = 'Anant Narayanan',
       author_email = 'anant@kix.in',
       url = 'http://libburn.pykix.org/',
       long_description = '''
        Really stupid bindings for libburn-SVN
       ''',
       ext_modules = [module])

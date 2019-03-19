import generic

def options(opt):
    generic._options(opt, 'libdaxproto')

def configure(cfg):
    generic._configure(cfg, 'libdaxproto', incs=('daxproto.h',), libs=('daxproto',),
                       pcname='libdaxproto',
                       uses = 'LIBDAXPROTO', mandatory=True)

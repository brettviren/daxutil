import generic

def options(opt):
    generic._options(opt, 'libczmq')

def configure(cfg):
    generic._configure(cfg, 'libczmq', incs=('czmq.h',), libs=('czmq',),
                       pcname='libczmq',
                       uses = 'LIBZMQ', mandatory=True)

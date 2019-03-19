#!/bin/bash

# assume an installation location in ~/opt.

waf configure \
    --with-ptmp=$HOME/opt/ptmp \
    --with-libzmq=$HOME/opt/zmq \
    --with-libczmq=$HOME/opt/zmq \
    --with-libzyre=$HOME/opt/zmq \
    --prefix=$HOME/opt/ptmp

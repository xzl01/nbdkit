#!/usr/bin/env bash
# nbdkit
# Copyright Red Hat
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright
# notice, this list of conditions and the following disclaimer in the
# documentation and/or other materials provided with the distribution.
#
# * Neither the name of Red Hat nor the names of its contributors may be
# used to endorse or promote products derived from this software without
# specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY RED HAT AND CONTRIBUTORS ''AS IS'' AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
# THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RED HAT OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
# USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
# OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
# SUCH DAMAGE.

# Test that pwrite is allowed to ignore stdin (in nbdkit >= 1.36).

source ./functions.sh
set -e
set -x

requires_plugin sh
requires_nbdsh_uri

sock=$(mktemp -u /tmp/nbdkit-test-sock.XXXXXX)
pid=sh-pwrite-ignore-stdin.pid
files="$sock $pid"
rm -f $files
cleanup_fn rm -f $files

start_nbdkit -P $pid -U $sock sh - <<'EOF'
case "$1" in
    can_write)
        exit 0
        ;;
    pwrite)
        # Always ignore the input.  If the offset >= 32M return an error.
        if [ $4 -ge 33554432 ]; then
            echo 'ENOSPC Out of space' >&2
            exit 1
        fi
        ;;
    get_size)
        echo 64M
        ;;
    pread)
        ;;
    *) exit 2 ;;
esac
EOF

nbdsh -u "nbd+unix://?socket=$sock" -c '
import errno

buf = bytearray(16*1024*1024)
h.pwrite(buf, 0)

try:
    h.pwrite(buf, 32*1024*1024)
    assert False
except nbd.Error as ex:
    # Expect an ENOSPC error here.
    assert ex.errnum == errno.ENOSPC
'

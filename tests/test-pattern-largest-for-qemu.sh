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

# Test the pattern plugin with the largest possible size supported
# by qemu and nbdkit.

source ./functions.sh
set -e
set -x

requires qemu-io --version

sock=$(mktemp -u /tmp/nbdkit-test-sock.XXXXXX)
files="pattern-largest-for-qemu.out pattern-largest-for-qemu.pid $sock"
rm -f $files
cleanup_fn rm -f $files

# Run nbdkit with pattern plugin.
start_nbdkit -P pattern-largest-for-qemu.pid -U $sock pattern $largest_qemu_disk

# Ideally we would read the last sector only.  However qemu-io has a
# bug where offsets are calculated using floating point numbers and
# silently truncated to 53 bits of precision.  Reading the last 1024
# bytes happens to give the correct result here.
# https://www.mail-archive.com/qemu-devel@nongnu.org/msg770572.html
qemu-io -r -f raw "nbd+unix://?socket=$sock" \
        -c 'r -v 9223372035781032960 1024' | grep -E '^[[:xdigit:]]+:' > pattern-largest-for-qemu.out
if [ "$(cat pattern-largest-for-qemu.out)" != "7fffffffbffffc00:  7f ff ff ff bf ff fc 00 7f ff ff ff bf ff fc 08  ................
7fffffffbffffc10:  7f ff ff ff bf ff fc 10 7f ff ff ff bf ff fc 18  ................
7fffffffbffffc20:  7f ff ff ff bf ff fc 20 7f ff ff ff bf ff fc 28  ................
7fffffffbffffc30:  7f ff ff ff bf ff fc 30 7f ff ff ff bf ff fc 38  .......0.......8
7fffffffbffffc40:  7f ff ff ff bf ff fc 40 7f ff ff ff bf ff fc 48  ...............H
7fffffffbffffc50:  7f ff ff ff bf ff fc 50 7f ff ff ff bf ff fc 58  .......P.......X
7fffffffbffffc60:  7f ff ff ff bf ff fc 60 7f ff ff ff bf ff fc 68  ...............h
7fffffffbffffc70:  7f ff ff ff bf ff fc 70 7f ff ff ff bf ff fc 78  .......p.......x
7fffffffbffffc80:  7f ff ff ff bf ff fc 80 7f ff ff ff bf ff fc 88  ................
7fffffffbffffc90:  7f ff ff ff bf ff fc 90 7f ff ff ff bf ff fc 98  ................
7fffffffbffffca0:  7f ff ff ff bf ff fc a0 7f ff ff ff bf ff fc a8  ................
7fffffffbffffcb0:  7f ff ff ff bf ff fc b0 7f ff ff ff bf ff fc b8  ................
7fffffffbffffcc0:  7f ff ff ff bf ff fc c0 7f ff ff ff bf ff fc c8  ................
7fffffffbffffcd0:  7f ff ff ff bf ff fc d0 7f ff ff ff bf ff fc d8  ................
7fffffffbffffce0:  7f ff ff ff bf ff fc e0 7f ff ff ff bf ff fc e8  ................
7fffffffbffffcf0:  7f ff ff ff bf ff fc f0 7f ff ff ff bf ff fc f8  ................
7fffffffbffffd00:  7f ff ff ff bf ff fd 00 7f ff ff ff bf ff fd 08  ................
7fffffffbffffd10:  7f ff ff ff bf ff fd 10 7f ff ff ff bf ff fd 18  ................
7fffffffbffffd20:  7f ff ff ff bf ff fd 20 7f ff ff ff bf ff fd 28  ................
7fffffffbffffd30:  7f ff ff ff bf ff fd 30 7f ff ff ff bf ff fd 38  .......0.......8
7fffffffbffffd40:  7f ff ff ff bf ff fd 40 7f ff ff ff bf ff fd 48  ...............H
7fffffffbffffd50:  7f ff ff ff bf ff fd 50 7f ff ff ff bf ff fd 58  .......P.......X
7fffffffbffffd60:  7f ff ff ff bf ff fd 60 7f ff ff ff bf ff fd 68  ...............h
7fffffffbffffd70:  7f ff ff ff bf ff fd 70 7f ff ff ff bf ff fd 78  .......p.......x
7fffffffbffffd80:  7f ff ff ff bf ff fd 80 7f ff ff ff bf ff fd 88  ................
7fffffffbffffd90:  7f ff ff ff bf ff fd 90 7f ff ff ff bf ff fd 98  ................
7fffffffbffffda0:  7f ff ff ff bf ff fd a0 7f ff ff ff bf ff fd a8  ................
7fffffffbffffdb0:  7f ff ff ff bf ff fd b0 7f ff ff ff bf ff fd b8  ................
7fffffffbffffdc0:  7f ff ff ff bf ff fd c0 7f ff ff ff bf ff fd c8  ................
7fffffffbffffdd0:  7f ff ff ff bf ff fd d0 7f ff ff ff bf ff fd d8  ................
7fffffffbffffde0:  7f ff ff ff bf ff fd e0 7f ff ff ff bf ff fd e8  ................
7fffffffbffffdf0:  7f ff ff ff bf ff fd f0 7f ff ff ff bf ff fd f8  ................
7fffffffbffffe00:  7f ff ff ff bf ff fe 00 7f ff ff ff bf ff fe 08  ................
7fffffffbffffe10:  7f ff ff ff bf ff fe 10 7f ff ff ff bf ff fe 18  ................
7fffffffbffffe20:  7f ff ff ff bf ff fe 20 7f ff ff ff bf ff fe 28  ................
7fffffffbffffe30:  7f ff ff ff bf ff fe 30 7f ff ff ff bf ff fe 38  .......0.......8
7fffffffbffffe40:  7f ff ff ff bf ff fe 40 7f ff ff ff bf ff fe 48  ...............H
7fffffffbffffe50:  7f ff ff ff bf ff fe 50 7f ff ff ff bf ff fe 58  .......P.......X
7fffffffbffffe60:  7f ff ff ff bf ff fe 60 7f ff ff ff bf ff fe 68  ...............h
7fffffffbffffe70:  7f ff ff ff bf ff fe 70 7f ff ff ff bf ff fe 78  .......p.......x
7fffffffbffffe80:  7f ff ff ff bf ff fe 80 7f ff ff ff bf ff fe 88  ................
7fffffffbffffe90:  7f ff ff ff bf ff fe 90 7f ff ff ff bf ff fe 98  ................
7fffffffbffffea0:  7f ff ff ff bf ff fe a0 7f ff ff ff bf ff fe a8  ................
7fffffffbffffeb0:  7f ff ff ff bf ff fe b0 7f ff ff ff bf ff fe b8  ................
7fffffffbffffec0:  7f ff ff ff bf ff fe c0 7f ff ff ff bf ff fe c8  ................
7fffffffbffffed0:  7f ff ff ff bf ff fe d0 7f ff ff ff bf ff fe d8  ................
7fffffffbffffee0:  7f ff ff ff bf ff fe e0 7f ff ff ff bf ff fe e8  ................
7fffffffbffffef0:  7f ff ff ff bf ff fe f0 7f ff ff ff bf ff fe f8  ................
7fffffffbfffff00:  7f ff ff ff bf ff ff 00 7f ff ff ff bf ff ff 08  ................
7fffffffbfffff10:  7f ff ff ff bf ff ff 10 7f ff ff ff bf ff ff 18  ................
7fffffffbfffff20:  7f ff ff ff bf ff ff 20 7f ff ff ff bf ff ff 28  ................
7fffffffbfffff30:  7f ff ff ff bf ff ff 30 7f ff ff ff bf ff ff 38  .......0.......8
7fffffffbfffff40:  7f ff ff ff bf ff ff 40 7f ff ff ff bf ff ff 48  ...............H
7fffffffbfffff50:  7f ff ff ff bf ff ff 50 7f ff ff ff bf ff ff 58  .......P.......X
7fffffffbfffff60:  7f ff ff ff bf ff ff 60 7f ff ff ff bf ff ff 68  ...............h
7fffffffbfffff70:  7f ff ff ff bf ff ff 70 7f ff ff ff bf ff ff 78  .......p.......x
7fffffffbfffff80:  7f ff ff ff bf ff ff 80 7f ff ff ff bf ff ff 88  ................
7fffffffbfffff90:  7f ff ff ff bf ff ff 90 7f ff ff ff bf ff ff 98  ................
7fffffffbfffffa0:  7f ff ff ff bf ff ff a0 7f ff ff ff bf ff ff a8  ................
7fffffffbfffffb0:  7f ff ff ff bf ff ff b0 7f ff ff ff bf ff ff b8  ................
7fffffffbfffffc0:  7f ff ff ff bf ff ff c0 7f ff ff ff bf ff ff c8  ................
7fffffffbfffffd0:  7f ff ff ff bf ff ff d0 7f ff ff ff bf ff ff d8  ................
7fffffffbfffffe0:  7f ff ff ff bf ff ff e0 7f ff ff ff bf ff ff e8  ................
7fffffffbffffff0:  7f ff ff ff bf ff ff f0 7f ff ff ff bf ff ff f8  ................" ]
then
    echo "$0: unexpected pattern:"
    cat pattern-largest-for-qemu.out
    exit 1
fi
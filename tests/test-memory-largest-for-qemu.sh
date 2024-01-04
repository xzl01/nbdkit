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

# Test the memory plugin with the largest possible size supported
# by qemu and nbdkit.

source ./functions.sh
set -e
set -x

requires qemu-io --version

sock=$(mktemp -u /tmp/nbdkit-test-sock.XXXXXX)
files="memory-largest-for-qemu.out memory-largest-for-qemu.pid $sock"
rm -f $files
cleanup_fn rm -f $files

# Run nbdkit with memory plugin.
start_nbdkit -P memory-largest-for-qemu.pid -U $sock memory $largest_qemu_disk

# Write some stuff to the beginning, middle and end.
#
# Ideally we would write the last sector only.  However qemu-io has a
# bug where offsets are calculated using floating point numbers and
# silently truncated to 53 bits of precision.  Reading the last 1024
# bytes happens to give the correct result here.
# https://www.mail-archive.com/qemu-devel@nongnu.org/msg770572.html
qemu-io -f raw "nbd+unix://?socket=$sock" \
        -c 'w -P 1 0 512' \
        -c 'w -P 2 1000000001 65536' \
        -c 'w -P 3 9223372035781032960 1024'

qemu-io -r -f raw "nbd+unix://?socket=$sock" \
        -c 'r -v 0 512' | grep -E '^[[:xdigit:]]+:' > memory-largest-for-qemu.out
if [ "$(cat memory-largest-for-qemu.out)" != "00000000:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000010:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000020:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000030:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000040:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000050:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000060:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000070:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000080:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000090:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000000a0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000000b0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000000c0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000000d0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000000e0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000000f0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000100:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000110:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000120:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000130:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000140:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000150:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000160:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000170:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000180:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
00000190:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000001a0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000001b0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000001c0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000001d0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000001e0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................
000001f0:  01 01 01 01 01 01 01 01 01 01 01 01 01 01 01 01  ................" ]
then
    echo "$0: unexpected memory:"
    cat memory-largest-for-qemu.out
    exit 1
fi

qemu-io -r -f raw "nbd+unix://?socket=$sock" \
        -c 'r -v 1000000001 512' | grep -E '^[[:xdigit:]]+:' > memory-largest-for-qemu.out
if [ "$(cat memory-largest-for-qemu.out)" != "3b9aca01:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca11:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca21:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca31:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca41:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca51:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca61:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca71:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca81:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9aca91:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acaa1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acab1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acac1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acad1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acae1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acaf1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb01:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb11:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb21:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb31:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb41:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb51:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb61:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb71:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb81:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acb91:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acba1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acbb1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acbc1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acbd1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acbe1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................
3b9acbf1:  02 02 02 02 02 02 02 02 02 02 02 02 02 02 02 02  ................" ]
then
    echo "$0: unexpected memory:"
    cat memory-largest-for-qemu.out
    exit 1
fi

# This block of memory was not set, so it should read back as zeroes.
qemu-io -r -f raw "nbd+unix://?socket=$sock" \
        -c 'r -v 2000000000 512' | grep -E '^[[:xdigit:]]+:' > memory-largest-for-qemu.out
if [ "$(cat memory-largest-for-qemu.out)" != "77359400:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359410:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359420:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359430:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359440:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359450:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359460:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359470:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359480:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359490:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773594a0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773594b0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773594c0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773594d0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773594e0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773594f0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359500:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359510:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359520:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359530:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359540:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359550:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359560:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359570:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359580:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
77359590:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773595a0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773595b0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773595c0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773595d0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773595e0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................
773595f0:  00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00  ................" ]
then
    echo "$0: unexpected memory:"
    cat memory-largest-for-qemu.out
    exit 1
fi

qemu-io -r -f raw "nbd+unix://?socket=$sock" \
        -c 'r -v 9223372035781032960 1024' | grep -E '^[[:xdigit:]]+:' > memory-largest-for-qemu.out
if [ "$(cat memory-largest-for-qemu.out)" != "7fffffffbffffc00:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc10:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc20:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc30:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc40:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc50:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc60:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc70:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc80:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffc90:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffca0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffcb0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffcc0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffcd0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffce0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffcf0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd00:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd10:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd20:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd30:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd40:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd50:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd60:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd70:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd80:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffd90:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffda0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffdb0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffdc0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffdd0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffde0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffdf0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe00:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe10:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe20:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe30:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe40:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe50:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe60:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe70:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe80:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffe90:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffea0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffeb0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffec0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffed0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffee0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffef0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff00:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff10:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff20:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff30:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff40:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff50:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff60:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff70:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff80:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffff90:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffffa0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffffb0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffffc0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffffd0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbfffffe0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................
7fffffffbffffff0:  03 03 03 03 03 03 03 03 03 03 03 03 03 03 03 03  ................" ]
then
    echo "$0: unexpected memory:"
    cat memory-largest-for-qemu.out
    exit 1
fi
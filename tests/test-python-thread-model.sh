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

source ./functions.sh
set -e
set -x

script="$abs_top_srcdir/tests/python-thread-model.py"
test -f "$script"

skip_if_valgrind "because Python code leaks memory"
requires nbdsh --version

# Check the plugin is loadable.
nbdkit python $script --dump-plugin

# This test only works on modern Linux (with pipe2, accept4 etc) where
# we are able to issue parallel requests.  Other platforms have more
# restrictive thread models.
requires sh -c "nbdkit python $script --dump-plugin |
                grep '^thread_model=parallel'"

pid=test-python-thread-model.pid
sock=$(mktemp -u /tmp/nbdkit-test-sock.XXXXXX)
files="$out $pid $sock"
rm -f $files
cleanup_fn rm -f $files

start_nbdkit -P $pid -U $sock python $script

export sock
nbdsh -c '
import os
import time

h.connect_unix(os.environ["sock"])

# We should be able to issue multiple requests in parallel,
# and the total time taken should not be much more than 10 seconds
# because all sleeps in the plugin should happen in parallel.
start_t = time.time()
for i in range(10):
    buf = nbd.Buffer(512)
    h.aio_pread(buf, 0)

while h.aio_in_flight() > 0:
    h.poll(-1)
end_t = time.time()

t = end_t - start_t
print(t)

# Since we launched 10 requests, if we serialized on them we
# would have waited at least 100 seconds.  We would expect to
# wait around 10 seconds, but for flexibility on slow servers
# any test < 100 should be fine.
assert t <= 50
'

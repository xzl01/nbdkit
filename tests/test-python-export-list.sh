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

script=$abs_top_srcdir/tests/python-export-list.py
test -f "$script"

skip_if_valgrind "because Python code leaks memory"
requires_nbdinfo
requires_nbdsh_uri
requires nbdsh -c 'print(h.set_full_info)'
requires jq --version

pid=test-python-export-list.pid
sock=$(mktemp -u /tmp/nbdkit-test-sock.XXXXXX)
out=test-python-export-list.out
files="$pid $sock $out"
rm -f $files
cleanup_fn rm -f $files

start_nbdkit -P $pid -U $sock python $script

nbdinfo --list --json nbd+unix://\?socket=$sock > $out
cat $out
# libnbd 1.4.0 differs from 1.4.1 on whether --list grabs descriptions
result=$(jq -c '[.exports[] | [."export-name", .description]]' $out)
test "$result" = '[["hello","world"],["name only",null]]' ||
  test "$result" = '[["hello","world"],["name only","=name only="]]'

nbdinfo --json nbd+unix://\?socket=$sock > $out
cat $out
diff -u <(jq -c '[.exports[] | [."export-name", .description]]' $out) \
     <(printf %s\\n '[["hello","=hello="]]')

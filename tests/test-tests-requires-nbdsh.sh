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

# Check that tests that use nbdsh declare 'requires_nbdsh_uri' and/or
# 'requires nbdsh --version'.
#
# XXX We could enhance this test to see if the test uses URIs, or we
# could just unilaterally set a minimum version of nbdsh.

source ./functions.sh
set -e

# Try to get a list of tests written in shell script.  An good
# approximation is to see which ones include 'functions.sh'.
tests="$( grep -l functions.sh $srcdir/test-*.sh )"

errors=0

for t in $tests; do
    echo checking $t ...
    if grep -sq -- "nbdsh" $t; then
        if ! grep -sq -- "requires.*nbdsh" $t; then
            echo "error: $t: test uses nbdsh but does not declare 'requires_nbdsh_uri' or 'requires nbdsh --version'"
            ((errors++)) ||:
        fi
    else # and the negative:
        if grep -sq -- "requires.*nbdsh" $t; then
            echo "error: $t: test does not use nbdsh but declares that it requires nbdsh"
            ((errors++)) ||:
        fi
    fi
done

if [ "$errors" -ge 1 ]; then exit 1; fi

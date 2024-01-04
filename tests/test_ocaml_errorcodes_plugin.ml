(* nbdkit
 * Copyright Red Hat
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 * * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * * Neither the name of Red Hat nor the names of its contributors may be
 * used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY RED HAT AND CONTRIBUTORS ''AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL RED HAT OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
 * USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *)

open Unix

let sector_size = 512

let open_connection _ = ()

let get_size () = Int64.of_int (6 * sector_size)

let pread () count offset _ =
  (* Depending on the sector requested (offset), return a different
   * error code.
   *)
  match (Int64.to_int offset) / sector_size with
  | 0 -> (* good, return data *) String.make count '\000'
  | 1 -> NBDKit.set_error EPERM;     failwith "EPERM"
  | 2 -> NBDKit.set_error EIO;       failwith "EIO"
  | 3 -> NBDKit.set_error ENOMEM;    failwith "ENOMEM"
  | 4 -> NBDKit.set_error ESHUTDOWN; failwith "ESHUTDOWN"
  | 5 -> NBDKit.set_error EINVAL;    failwith "EINVAL"
  | _ -> assert false

let () =
  NBDKit.register_plugin
    ~name:    "test-ocaml-errorcodes"
    ~version: (NBDKit.version ())

    ~open_connection
    ~get_size
    ~pread
    ()

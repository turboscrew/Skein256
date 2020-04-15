By Juha Aaltonen 15.4.2020

# SKEIN256 HASH
## As described in paper "skein 1.3"
<http://www.skein-hash.info/sites/default/files/skein1.3.pdf>

### Description
This is a "conceptually closer" C implementation of SKEIN256 hash.
It doesn't depend on any OS or C compiler.
It was really meant for understanding the paper.

It only supports data of full bytes and only three kinds of UBI blocks are supported:
configuration, message and output.

The code is public domain.



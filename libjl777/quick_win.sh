export PATH=$PWD/mxe/usr/bin:$PATH
echo $PATH
echo ">>>>>>>>>>>>>>>>>>building SuperNET.exe"
i686-w64-mingw32.static-gcc -o SuperNET.exe -w SuperNET.c picoc.c table.c lex.c parse.c expression.c heap.c type.c variable.c clibrary.c platform.c include.c platform/platform_unix.c platform/library_unix.c cstdlib/stdio.c cstdlib/math.c cstdlib/string.c cstdlib/stdlib.c cstdlib/time.c cstdlib/errno.c cstdlib/ctype.c cstdlib/stdbool.c cstdlib/unistd.c libgfshare.c libjl777.c gzip/adler32.c gzip/crc32.c gzip/gzclose.c gzip/gzread.c gzip/infback.c gzip/inflate.c gzip/trees.c gzip/zutil.c gzip/compress.c gzip/deflate.c gzip/gzlib.c gzip/gzwrite.c gzip/inffast.c  gzip/inftrees.c gzip/uncompr.c libtom/yarrow.c libtom/aes.c libtom/cast5.c libtom/khazad.c libtom/rc2.c libtom/safer.c libtom/skipjack.c libtom/aes_tab.c libtom/crypt_argchk.c libtom/kseed.c libtom/rc5.c libtom/saferp.c libtom/twofish.c libtom/anubis.c libtom/des.c libtom/multi2.c libtom/rc6.c libtom/safer_tab.c libtom/twofish_tab.c libtom/blowfish.c libtom/kasumi.c libtom/noekeon.c libtom/rmd160.c libtom/sha256.c libtom/xtea.c libs/libdb-win.a libs/libuv-win.a -lwebsockets libs/libminiupnpc-win.a -DSTATICLIB -DMINIUPNP_STATICLIB -lssl -lcrypto -lpthread -lssh2 -lm -lws2_32 -lwsock32 -lgdi32 -DCURL_STATICLIB -lgcrypt -lwldap32 -liconv -lintl -lnettle -lpsapi -liphlpapi -lcurl -lws2_32 -lwldap32
strip SuperNET.exe
echo '>>>>>>>>>>>>>>>>>>finished'

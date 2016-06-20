dnl $Id$
dnl config.m4 for extension toml

dnl Comments in this file start with the string 'dnl'.
dnl Remove where necessary. This file will not work
dnl without editing.

dnl If your extension references something external, use with:

PHP_ARG_WITH(toml, for toml support,
Make sure that the comment is aligned:
[  --with-toml             Include toml support])

dnl Otherwise use enable:

PHP_ARG_ENABLE(toml, whether to enable toml support,
Make sure that the comment is aligned:
[  --enable-toml           Enable toml support])

if test "$PHP_TOML" != "no"; then
  dnl Write more examples of tests here...

  dnl # --with-toml -> check with-path
  dnl SEARCH_PATH="/usr/local /usr"     # you might want to change this
  dnl SEARCH_FOR="/include/toml.h"  # you most likely want to change this
  dnl if test -r $PHP_TOML/$SEARCH_FOR; then # path given as parameter
  dnl   TOML_DIR=$PHP_TOML
  dnl else # search default path list
  dnl   AC_MSG_CHECKING([for toml files in default path])
  dnl   for i in $SEARCH_PATH ; do
  dnl     if test -r $i/$SEARCH_FOR; then
  dnl       TOML_DIR=$i
  dnl       AC_MSG_RESULT(found in $i)
  dnl     fi
  dnl   done
  dnl fi
  dnl
  dnl if test -z "$TOML_DIR"; then
  dnl   AC_MSG_RESULT([not found])
  dnl   AC_MSG_ERROR([Please reinstall the toml distribution])
  dnl fi

  dnl # --with-toml -> add include path
  dnl PHP_ADD_INCLUDE($TOML_DIR/include)

  dnl # --with-toml -> check for lib and symbol presence
  dnl LIBNAME=toml # you may want to change this
  dnl LIBSYMBOL=toml # you most likely want to change this 

  dnl PHP_CHECK_LIBRARY($LIBNAME,$LIBSYMBOL,
  dnl [
  dnl   PHP_ADD_LIBRARY_WITH_PATH($LIBNAME, $TOML_DIR/$PHP_LIBDIR, TOML_SHARED_LIBADD)
  dnl   AC_DEFINE(HAVE_TOMLLIB,1,[ ])
  dnl ],[
  dnl   AC_MSG_ERROR([wrong toml lib version or lib not found])
  dnl ],[
  dnl   -L$TOML_DIR/$PHP_LIBDIR -lm
  dnl ])
  dnl
  PHP_SUBST(TOML_SHARED_LIBADD)

  PHP_NEW_EXTENSION(toml, toml.c, $ext_shared,, -DZEND_ENABLE_STATIC_TSRMLS_CACHE=1)
fi

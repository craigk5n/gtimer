dnl Process this file with autoconf to produce a configure script.
AC_INIT(gtimer,1.1.7)
AC_CONFIG_SRCDIR(main.c)
AM_INIT_AUTOMAKE

dnl Checks for programs.
AC_PROG_CC

dnl Checks for header files.
dnl AC_HEADER_STDC
AC_CHECK_HEADERS(fcntl.h sys/time.h sys/types unistd.h string.h libintl.h)

dnl ALL_LINGUAS="fr"
dnl ud_GNU_GETTEXT
dnl AC_LINK_FILES($nls_cv_header_libgt, $nls_cv_header_intl)

AM_PATH_GTK_2_0(2.4.0,,AC_MSG_ERROR(No GTK found. Set the path right or install it.))

dnl #if test "x$GTK_MAJOR_VERSION" = "x1" -a "x$GTK_MAJOR_VERSION" = "x0"; then
dnl #  echo "Configure found an old version of GTK. While gtimer will still work fine,"
dnl # echo "it would be best to upgrade it."
dnl #  AC_DEFINE(GTK_VERSION_20)
dnl #fi

dnl ScreenSaver extension support
ssext=yes
AC_ARG_ENABLE(shm,
[  --disable-ssext           disable usage of ScreenSaver extension],
                ssext=$enableval, ssext=yes)

if test "$ssext" = yes; then
  AC_CHECK_LIB(Xss, XScreenSaverQueryInfo, [GTK_LIBS="-lXss -lXext $GTK_LIBS"
    ok=yes
    AC_DEFINE(HAVE_SCREEN_SAVER_EXT)], ok=no, $GTK_LIBS)
fi


UILIBS=$GTK_LIBS
UIFLAGS="$GTK_CFLAGS $GTK_VERSION_FLAGS"

AC_SUBST(UILIBS)
AC_SUBST(UIFLAGS)

AC_OUTPUT(Makefile)

dnl AC_OUTPUT([Makefile intl/Makefile po/Makefile.in],

dnl [sed -e "/POTFILES =/r po/POTFILES" po/Makefile.in > po/Makefile])


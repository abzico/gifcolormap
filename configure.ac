AC_PREREQ([2.69])
AC_INIT([gifcolormap], [0.1], [wasin@abzi.co])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])
AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

AM_PROG_AR

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET

# Checks for libraries.
AC_SEARCH_LIBS([DGifCloseFile], [gif], [], [AC_MSG_ERROR("Not found libgif. Please install it on your system https://sourceforge.net/projects/giflib/")])

# Checks for header files.
AC_CHECK_HEADERS([stdio.h stdbool.h stdlib.h stdarg.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
AC_FUNC_MALLOC
AC_CHECK_FUNCS([strtol])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT

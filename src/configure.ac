AC_INIT(ftpd.c)

DCAS='sh-elf-as -little'
DCLD='sh-elf-ld -EL'
DCCC='sh-elf-gcc -ml -m4-single-only'
DCAR='sh-elf-ar'
DCOBJCOPY='sh-elf-objcopy'
DCCFLAGS='-Os'
AC_SUBST(DCAS)
AC_SUBST(DCLD)
AC_SUBST(DCCC)
AC_SUBST(DCAR)
AC_SUBST(DCOBJCOPY)
AC_SUBST(DCCFLAGS)

AC_ARG_WITH(libronin, [  --with-libronin=path    specify path to libronin installation],
            RONINDIR="$withval",
	    RONINDIR=/usr/local/ronin)

AC_SUBST(RONINDIR)

AC_OUTPUT(Makefile)

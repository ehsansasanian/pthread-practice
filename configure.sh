#!/bin/sh
debug=no
check=unknown
optimize=unknown
sanitizeaddress=no
sanitizethread=no
die () {
  echo "configure.sh: error: $*" 1>&2
  exit 1
}
while [ $# -gt 0 ]
do
  case $1 in
    -h|--help) 
cat <<EOF
usage: configure [ <option> ... ]

where <option> is one of the following

  -h | --help              print this command line summary and exit
  -g | --debug             compile with debugging information
  -c | --check             compile with assertion checking (default for '-g')
  -O | --optimize          compile with optimization (default if not '-g')
  -m | -fsanitize=address  use memory sanitizer
  -t | -fsanitize=threads  use tread sanitizer

Compiler is set through the environment variable 'CC' (default 'gcc').
EOF
    exit 0
    ;;
  -g | --debug)            debug=yes;;
  -c | --check)            check=yes;;
  -O | --optimize)         optimize=yes;;
  -m | -fsanitize=address) sanitizeaddress=yes;;
  -t | -fsanitize=threads) sanitizethread=yes;;
  esac
  shift
done

if [ x"$CC" = x ]
then
  CC="gcc"
fi

CFLAGS="-Wall"

if [ $optimize = unknown ]
then
  if [ $debug = yes ]
  then
    optimize=no
  else
    optimize=yes
  fi
fi

if [ $check = unknown ]
then
  if [ $optimize = yes ]
  then
    check=no
  else
    check=yes
  fi
fi

[ $debug = yes ] && CFLAGS="$CFLAGS -ggdb3"
[ $check = no ] && CFLAGS="$CFLAGS -DNDEBUG"
[ $optimize = yes ] && CFLAGS="$CFLAGS -O3"
[ $sanitizeaddress = yes ] && CFLAGS="$CFLAGS -fsanitize=address"
[ $sanitizethread = yes ] && CFLAGS="$CFLAGS -fsanitize=thread"

CFLAGS="$CFLAGS -pthread"

echo "$CC $CFLAGS"

rm -f makefile
sed -e "s,@CC@,$CC," -e "s,@CFLAGS@,$CFLAGS," makefile.in > makefile

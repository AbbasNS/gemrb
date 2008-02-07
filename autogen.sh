#!/bin/sh

# $Id$

# Gentoo and Mandrake specific flags
export WANT_AUTOMAKE="1.7"
export WANT_AUTOCONF="2.5"

# FreeBSD favourite paths
#export CXXFLAGS="-ggdb -I/usr/local/include"
#export LDFLAGS="-ggdb -L/usr/local/lib"

if [ "$1" = "" ]; then
  dest=$HOME/GemRB
else
  dest=$1
  shift
fi

if [ -n "$ACLOCAL" ];
then
  my_aclocal=$ACLOCAL
else
  for file in aclocal aclocal-1.7 aclocal-1.8 aclocal-1.9 aclocal-1.10 aclocal-1.11; do
    version=`$file --version | sed -n '1 { s/^[^ ]* (.*) //; s/ .*$//; s,\.,,g; p}'`
    if [ "$version" -gt 17 ];
    then
      my_aclocal=$file
      break
    fi
  done
fi

if [ -z "$my_aclocal" ];
then 
  echo "***************************************************************"
  echo
  echo "Aclocal version >= 1.7 is required. If it is already installed,"
  echo "set enviroment variable ACLOCAL to point to it."
  echo "(for example in bash do: \"export ACLOCAL=/pathto/aclocal-1.7\")"
  echo
  echo "***************************************************************"
  exit 1
fi

if [ -n "$AUTOHEADER" ];
then
  my_autoheader=$AUTOHEADER
else
  for file in autoheader autoheader-2.57 autoheader-2.58 autoheader-2.59 autoheader-2.60 autoheader-2.61 autoheader-2.62; do
    version=`$file --version | sed -n '1 { s/^[^ ]* (.*) //; s/ .*$//; s,\.,,g; p}'`
    if [ "$version" -gt 257 ];
    then
      my_autoheader=$file
      break
    fi
  done
fi

if [ -z "$my_autoheader" ];
then
  echo "*********************************************************************"
  echo
  echo "Autoheader version >= 2.57 is required. If it is already installed,"
  echo "set enviroment variable AUTOHEADER to point to it."
  echo "(for example in bash do: \"export AUTOHEADER=/pathto/autoheader-2.58\")"
  echo
  echo "*********************************************************************"
  exit 1
fi

if [ -n "$LIBTOOLIZE" ];
then
  my_libtoolize=$LIBTOOLIZE
else
  for file in libtoolize; do
    version=`$file --version | sed -n '1 { s/^[^ ]* (.*) //; s/ .*$//; s,\.,,g; p}'`
    if [ "$version" -gt 15 ];
    then
      my_libtoolize=$file
      break
    fi
  done
fi

if [ -z "$my_libtoolize" ];
then 
  echo "***************************************************************"
  echo
  echo "Libtool version >= 1.5 is required. If it is already installed,"
  echo "set enviroment variable LIBTOOLIZE to point to it."
  echo "(for example in bash do: \"export LIBTOOLIZE=/pathto/libtoolize\")"
  echo
  echo "***************************************************************"
  exit 1
fi

if [ -n "$AUTOMAKE" ];
then
  my_automake=$AUTOMAKE
else
  for file in automake automake-1.7 automake-1.8 automake-1.9 automake-1.10 automake-1.11; do
    version=`$file --version | sed -n '1 { s/^[^ ]* (.*) //; s/ .*$//; s,\.,,g; p}'`
    if [ "$version" -gt 17 ];
    then
      my_automake=$file
      break
    fi
  done
fi

if [ -z "$my_automake" ];
then 
  echo "***************************************************************"
  echo
  echo "Automake version >= 1.7 is required. If it is already installed,"
  echo "set enviroment variable AUTOMAKE to point to it."
  echo "(for example in bash do: \"export AUTOMAKE=/pathto/automake-1.7\")"
  echo
  echo "***************************************************************"
  exit 1
fi

if [ -n "$AUTOCONF" ];
then
  my_autoconf=$AUTOCONF
else
  for file in autoconf autoconf-2.57 autoconf-2.58 autoconf-2.59 autoconf-2.60 autoconf-2.61 autoconf-2.62; do
    version=`$file --version | sed -n '1 { s/^[^ ]* (.*) //; s/ .*$//; s,\.,,g; p}'`
    if [ "$version" -gt 257 ];
    then
      my_autoconf=$file
      break
    fi
  done
fi

if [ -z "$my_autoconf" ];
then
  echo "*****************************************************************"
  echo
  echo "Autoconf version >= 2.58 is required. If it is already installed,"
  echo "set enviroment variable AUTOCONF to point to it."
  echo "(for example in bash do: \"export AUTOCONF=/pathto/autoconf-2.58\")"
  echo
  echo "*********************************************************************"
  exit 1
fi

echo Running libtoolize
$my_libtoolize --force || exit 1

echo Running aclocal
$my_aclocal || exit 1

echo Running autoconf
$my_autoconf || exit 1

echo Running autoheader
$my_autoheader || exit 1

echo Running automake
$my_automake --add-missing || exit 1

echo Running configure

cmd="./configure --prefix=$dest/ --bindir=$dest/ --sysconfdir=$dest/ --datadir=$dest/ --libdir=$dest/plugins/ --disable-subdirs"
$cmd

echo
echo "Configure was invoked as: $cmd"

#!/bin/bash

####################
# copy dylib files #
####################

export CARTAWORKHOME=`cd ../../../ && pwd`
echo $CARTAWORKHOME

cd $CARTAWORKHOME/CARTAvis
export CARTABUILDHOME=`pwd`

APPNAME=testProtoBuf
echo $APPNAME

CARTAAPP=build/cpp/$APPNAME/$APPNAME.app
echo $CARTAAPP

mkdir -p $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/

cp $CARTABUILDHOME/build/cpp/core/libcore.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/
# rm $CARTABUILDHOME/build/cpp/core/libcore.1.dylib

cp $CARTABUILDHOME/build/cpp/CartaLib/libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/
# rm $CARTABUILDHOME/build/cpp/CartaLib/libCartaLib.1.dylib

# need to rm for qt creator 4.2, otherwise when build+run together will result in core/libcore.1.dylib not able find out qwt

install_name_tool -change qwt.framework/Versions/6/qwt  $CARTABUILDHOME/ThirdParty/qwt-6.1.2/lib/qwt.framework/Versions/6/qwt  $CARTABUILDHOME/$CARTAAPP/Contents/MacOS/$APPNAME
install_name_tool -change qwt.framework/Versions/6/qwt  $CARTABUILDHOME/ThirdParty/qwt-6.1.2/lib/qwt.framework/Versions/6/qwt  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libcore.1.dylib

# not sure the effect of the below line, try comment

# install_name_tool -change libplugin.dylib    $CARTABUILDHOME/build/cpp/plugins/CasaImageLoader/libplugin.dylib                    $CARTABUILDHOME/build/cpp/plugins/ImageStatistics/libplugin.dylib

install_name_tool -change libcore.1.dylib      $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libcore.1.dylib      $CARTABUILDHOME/build/cpp/plugins/ImageStatistics/libplugin.dylib
install_name_tool -change libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libCartaLib.1.dylib  $CARTABUILDHOME/build/cpp/plugins/ImageStatistics/libplugin.dylib

install_name_tool -change libcore.1.dylib      $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libcore.1.dylib      $CARTABUILDHOME/$CARTAAPP/Contents/MacOS/$APPNAME
install_name_tool -change libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/MacOS/$APPNAME

install_name_tool -change libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libcore.1.dylib

for f in `find . -name libplugin.dylib`;  do install_name_tool -change libcore.1.dylib      $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libcore.1.dylib      $f; done
for f in `find . -name libplugin.dylib`;  do install_name_tool -change libCartaLib.1.dylib  $CARTABUILDHOME/$CARTAAPP/Contents/Frameworks/libCartaLib.1.dylib  $f; done
for f in `find . -name "*.dylib"`;        do install_name_tool -change libwcs.5.15.dylib    $CARTABUILDHOME/ThirdParty/wcslib/lib/libwcs.5.15.dylib            $f; echo $f; done

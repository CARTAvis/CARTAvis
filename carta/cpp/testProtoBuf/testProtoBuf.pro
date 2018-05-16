! include(../common.pri) {
  error( "Could not find the common.pri file!" )
}

QT      += network widgets xml

HEADERS += \
    ProtoBufHelper.h \
    CartaHelper.h

SOURCES += \
    ProtoBufHelper.cpp \
    CartaHelper.cpp \
    main.cpp

RESOURCES =

unix: LIBS += -L$$OUT_PWD/../core/ -lcore
unix: LIBS += -L$$OUT_PWD/../CartaLib/ -lCartaLib

DEPENDPATH += $$PROJECT_ROOT/core
DEPENDPATH += $$PROJECT_ROOT/CartaLib

QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/../CartaLib:\$$ORIGIN/../core\''

QWT_ROOT = $$absolute_path("../../../ThirdParty/qwt")

# Copies the given files to the destination directory
defineTest(copyToDestdir) {
    files = $$PWD/$$1

    for(FILE, files) {
        QMAKE_POST_LINK += $$QMAKE_COPY $$quote($$FILE) $$quote($$OUT_PWD)
    }

    message("pwd= $$PWD")

    export(QMAKE_POST_LINK)
}

copyToDestdir(example.sh)

INCLUDEPATH += ../core

## make protocol buffer codes .pb.h and .pb.cc
! include(proto_raster_image.pri) {
  error("Could not find the proto_raster_image.pri file!")
}

## link protocol buffer package headers and lib
INCLUDEPATH += $${PROTOBUFDIR}/include
LIBS +=-L$${PROTOBUFDIR}/lib -lprotobuf

unix:macx {
    QMAKE_LFLAGS += '-F$$QWT_ROOT/lib'
    LIBS +=-framework qwt
    PRE_TARGETDEPS += $$OUT_PWD/../core/libcore.dylib
}
else{
    QMAKE_LFLAGS += '-Wl,-rpath,\'$$QWT_ROOT/lib\''
    LIBS +=-L$$QWT_ROOT/lib -lqwt
    PRE_TARGETDEPS += $$OUT_PWD/../core/libcore.so
    QMAKE_RPATHDIR=$$OUT_PWD/../../../../CARTAvis-externals/ThirdParty/casa/trunk/linux/lib
}

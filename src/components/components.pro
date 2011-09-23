include (../../qt-components.pri)

TARGETPATH = Qt/labs/components.1.1
TEMPLATE = lib
TARGET = $$qtLibraryTarget(qtcomponentsplugin_1_1)
INCLUDEPATH += $$PWD $$PWD/models

win32|mac:!wince*:!win32-msvc:!macx-xcode:CONFIG += debug_and_release build_all
CONFIG += qt plugin
QT += declarative

DEFINES += QT_BUILD_COMPONENTS_LIB

QML_FILES += \
    qmldir \
    Checkable.qml \
    CheckableGroup.qml \
    CheckableGroup.js

symbian {
    TARGET.EPOCALLOWDLLDATA = 1
    TARGET.CAPABILITY = ALL -TCB
    TARGET.UID3 = 0x2003DF67
    MMP_RULES += SMPSAFE
    VERSION = 10.1.0

    stubsis = \
        "START EXTENSION app-services.buildstubsis" \
        "OPTION SISNAME qtcomponentsplugin_1_1_stub" \
        "OPTION SRCDIR ."\
        "END"
    BLD_INF_RULES.prj_extensions = stubsis

    vendor_info = \
            " " \
            "; Localised Vendor name" \
            "%{\"Nokia\"}" \
            " " \
            "; Unique Vendor name" \
            ":\"Nokia\"" \
            " "

    package.pkg_prerules += vendor_info
    DEPLOYMENT += package

    defBlock = \
        "$${LITERAL_HASH}ifdef WINSCW" \
        "DEFFILE  bwins/plugin_commonu.def" \
        "$${LITERAL_HASH}elif defined EABI" \
        "DEFFILE  eabi/plugin_commonu.def" \
        "$${LITERAL_HASH}endif"
    MMP_RULES += defBlock
}

HEADERS += qglobalenums.h

SOURCES += plugin.cpp

include(kernel/kernel.pri)

include(models/models.pri)

include(../../qml.pri)

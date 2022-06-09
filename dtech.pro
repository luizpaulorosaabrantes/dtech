QT += widgets serialport serialbus sql network
requires(qtConfig(combobox))
requires(qtConfig(tableview))

TARGET = dtech
TEMPLATE = app

SOURCES += \
    abnt-14522.c \
    abntframe.cpp \
    connections.cpp \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    absequipment.cpp \
    crc.c

HEADERS += \
    abnt-14522.h \
    abntframe.h \
    connections.h \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    absequipment.h \
    database.h \
    crc.h

FORMS += \
    abntframe.ui \
    connections.ui \
    mainwindow.ui \
    settingsdialog.ui \
    absequipment.ui

RESOURCES += \
    dtech.qrc

#target.path = $$[QT_INSTALL_EXAMPLES]/serialport/terminal
INSTALLS += target

DISTFILES +=

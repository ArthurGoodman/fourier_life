QT += core gui widgets

TARGET = fourier_life
TEMPLATE = app

LIBS += -L../fftw3 -lfftw3-3
INCLUDEPATH += ../fftw3

SOURCES += main.cpp \
        widget.cpp

HEADERS  += widget.h

QT += core gui widgets

TARGET = fourier_life
TEMPLATE = app

QMAKE_CXXFLAGS += -fopenmp

LIBS += -L../fftw3 -lfftw3f-3 -fopenmp
INCLUDEPATH += ../fftw3

SOURCES += main.cpp \
        widget.cpp

HEADERS  += widget.h

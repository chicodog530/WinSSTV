QT       += core gui widgets multimedia network serialport
QTPLUGIN += qtaudio_windows

TARGET = WinSSTV
TEMPLATE = app

# Define C++ standard
CONFIG += c++17 console
QMAKE_LFLAGS -= -static
QMAKE_LFLAGS += -Wl,-Bdynamic

# Include directories
INCLUDEPATH += src/core \
               src/core/sstv \
               src/core/sstv/modes \
               src/core/dsp \
               src/core/utils \
               src/core/sound \
               src/stubs \
               src/gui \
               C:/msys64/mingw64/include

# Source files
SOURCES += src/main.cpp \
           src/stubs/appglobal.cpp \
           src/stubs/dispatcher.cpp \
           src/gui/mainwindow.cpp \
           src/gui/waterfallwidget.cpp \
           src/gui/overlaywidget.cpp \
           src/gui/textoverlaydialog.cpp \
           src/gui/txworker.cpp \
           src/core/sstv/cw.cpp \
           src/core/sstv/sstvparam.cpp \
           src/core/sstv/sstvrx.cpp \
           src/core/sstv/sstvtx.cpp \
           src/core/sstv/syncprocessor.cpp \
           src/core/sstv/visfskid.cpp \
           src/core/sstv/modes/modeavt.cpp \
           src/core/sstv/modes/modebase.cpp \
           src/core/sstv/modes/modebw.cpp \
           src/core/sstv/modes/modegbr.cpp \
           src/core/sstv/modes/modegbr2.cpp \
           src/core/sstv/modes/modepd.cpp \
           src/core/sstv/modes/modergb.cpp \
           src/core/sstv/modes/moderobot1.cpp \
           src/core/sstv/modes/moderobot2.cpp \
           src/core/dsp/downsamplefilter.cpp \
           src/core/dsp/filter.cpp \
           src/core/dsp/filterparam.cpp \
           src/core/dsp/filters.cpp \
           src/core/dsp/synthes.cpp \
           src/core/utils/arraydumper.cpp \
           src/core/utils/color.cpp \
           src/core/utils/dirdialog.cpp \
           src/core/utils/fftcalc.cpp \
           src/core/utils/filewatcher.cpp \
           src/core/utils/hexconvertor.cpp \
           src/core/utils/hybridcrypt.cpp \
           src/core/utils/logging.cpp \
           src/core/utils/loggingparams.cpp \
           src/core/utils/macroexpansion.cpp \
           src/core/utils/reedsolomoncoder.cpp \
           src/core/utils/rs.cpp \
           src/core/utils/supportfunctions.cpp \
           src/core/sound/soundbase.cpp \
           src/core/sound/soundwindows.cpp \
           src/core/sound/wavio.cpp \
           src/core/utils/rigcontrol.cpp

# Header files
HEADERS += src/stubs/*.h \
           src/stubs/dispatcher.h \
           src/gui/mainwindow.h \
           src/gui/overlaywidget.h \
           src/gui/textoverlaydialog.h \
           src/gui/waterfallwidget.h \
           src/gui/txworker.h \
           src/core/utils/rigcontrol.h \
           src/core/sstv/visfskid.h \
           src/core/sstv/syncprocessor.h \
           src/core/sstv/sstvrx.h \
           src/core/sound/soundbase.h \
           src/core/sound/soundwindows.h \
           src/core/sstv/*.h \
           src/core/dsp/*.h \
           src/core/utils/*.h \
           src/core/sound/*.h

# Exclusions (if any specific files cause issues)
# Libraries
LIBS += -lfftw3 \
        -Wl,-Bdynamic -LC:/msys64/mingw64/lib -lhamlib -liphlpapi -lws2_32 -lwinmm -Wl,-Bstatic

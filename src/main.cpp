#include "appglobal.h"
#include "mainwindow.h"
#include <QApplication>
#include <QAudio>
#include <QAudioDeviceInfo>
#include <QDebug>
#include <QVector>
#include <QtPlugin>
#include <iostream>
#include <objbase.h>

// Q_IMPORT_PLUGIN(QWindowsAudioPlugin)

#include <QDateTime>
#include <QFile>
#include <QStandardPaths>
#include <QStyle>
#include <QTextStream>

void messageHandler(QtMsgType type, const QMessageLogContext &context,
                    const QString &msg) {
  QString txt;
  switch (type) {
  case QtDebugMsg:
    txt = QString("Debug: %1").arg(msg);
    break;
  case QtWarningMsg:
    txt = QString("Warning: %1").arg(msg);
    break;
  case QtCriticalMsg:
    txt = QString("Critical: %1").arg(msg);
    break;
  case QtFatalMsg:
    txt = QString("Fatal: %1").arg(msg);
    std::cerr << txt.toStdString() << std::endl;
    abort();
  case QtInfoMsg:
    txt = QString("Info: %1").arg(msg);
    break;
  }
  std::cout << txt.toStdString() << std::endl;
}

int main(int argc, char *argv[]) {
  HRESULT hr = CoInitialize(nullptr);
  if (FAILED(hr))
    std::cerr << "CoInitialize failed" << std::endl;
  else
    std::cout << "CoInitialize success" << std::endl;

  qInstallMessageHandler(messageHandler);
  QApplication a(argc, argv);

  // Apply Dark Mode
  a.setStyle("Fusion");
  QPalette darkPalette;
  darkPalette.setColor(QPalette::Window, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::WindowText, Qt::white);
  darkPalette.setColor(QPalette::Base, QColor(25, 25, 25));
  darkPalette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ToolTipBase, Qt::white);
  darkPalette.setColor(QPalette::ToolTipText, Qt::white);
  darkPalette.setColor(QPalette::Text, Qt::white);
  darkPalette.setColor(QPalette::Button, QColor(53, 53, 53));
  darkPalette.setColor(QPalette::ButtonText, Qt::white);
  darkPalette.setColor(QPalette::BrightText, Qt::red);
  darkPalette.setColor(QPalette::Link, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::Highlight, QColor(42, 130, 218));
  darkPalette.setColor(QPalette::HighlightedText, Qt::black);
  a.setPalette(darkPalette);
  a.setStyleSheet("QToolTip { color: #ffffff; background-color: #2a82da; "
                  "border: 1px solid white; }");

  // Check Audio Devices
  std::cout << "Checking audio devices..." << std::endl;
  auto devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  std::cout << "Audio Devices found: " << devices.count() << std::endl;
  for (const auto &d : devices) {
    std::cout << " - " << d.deviceName().toStdString() << std::endl;
  }

  // Register types for cross-thread signal/slot connections
  qRegisterMetaType<QVector<double>>("QVector<double>");
  // Initialize globals
  std::cout << "Calling globalInit..." << std::endl;
  globalInit();
  std::cout << "globalInit finished." << std::endl;

  qDebug() << "WinSSTV Core Initialized. Version:" << qsstvVersion;

  MainWindow w;
  mainWindowPtr = &w;
  w.show();
  std::cout << "Calling a.exec()..." << std::endl;
  return a.exec();
}

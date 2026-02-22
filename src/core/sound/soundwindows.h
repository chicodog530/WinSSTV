#ifndef SOUNDWINDOWS_H
#define SOUNDWINDOWS_H

#include "soundbase.h"
#include <QAudioDeviceInfo>
#include <QAudioInput>
#include <QAudioOutput>
#include <QIODevice>
#include <QList>
#include <QMutex>

class soundWindows : public soundBase {
  Q_OBJECT
public:
  explicit soundWindows(QObject *parent = nullptr);
  ~soundWindows();

  bool init(int samplerate) override;
  void getCardList() override;

  static QList<QAudioDeviceInfo> availableInputDevices();
  static QList<QAudioDeviceInfo> availableOutputDevices();

protected:
  int read(int &countAvailable) override;
  int write(unsigned int numFrames) override;
  void flushCapture() override;
  void flushPlayback() override;
  void closeDevices() override;
  void waitPlaybackEnd() override;
  void stopImmediate() override;

private:
  QAudioInput *audioInput;
  QAudioOutput *audioOutput;
  QIODevice *inputDevice;
  QIODevice *outputDevice;

  QAudioDeviceInfo inputDeviceInfo;
  QAudioDeviceInfo outputDeviceInfo;
  QMutex deviceMutex;
};

#endif // SOUNDWINDOWS_H

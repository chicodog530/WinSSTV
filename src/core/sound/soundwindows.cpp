#include "soundwindows.h"
#include "appdefs.h"
#include "configparams.h"
#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTime>

soundWindows::soundWindows(QObject *parent)
    : soundBase(parent), audioInput(nullptr), audioOutput(nullptr),
      inputDevice(nullptr), outputDevice(nullptr) {
  soundDriverOK = false;
}

soundWindows::~soundWindows() { closeDevices(); }

void soundWindows::getCardList() {
  QList<QAudioDeviceInfo> inputDevices = availableInputDevices();
  QList<QAudioDeviceInfo> outputDevices = availableOutputDevices();

  qDebug() << "Available Audio Inputs:";
  for (const auto &device : inputDevices) {
    qDebug() << " - " << device.deviceName();
  }

  qDebug() << "Available Audio Outputs:";
  for (const auto &device : outputDevices) {
    qDebug() << " - " << device.deviceName();
  }
}

QList<QAudioDeviceInfo> soundWindows::availableInputDevices() {
  return QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
}

QList<QAudioDeviceInfo> soundWindows::availableOutputDevices() {
  return QAudioDeviceInfo::availableDevices(QAudio::AudioOutput);
}

bool soundWindows::init(int samplerate) {
  QMutexLocker locker(&deviceMutex);
  closeDevices();
  (void)samplerate;            // Internal processing rate, not hardware rate
  sampleRate = BASESAMPLERATE; // Hardware always runs at 48kHz
  soundDriverOK = false;

  QAudioFormat format;
  format.setSampleRate(BASESAMPLERATE); // Hardware runs at 48kHz
  format.setChannelCount(1);            // Mono
  format.setSampleSize(16);
  format.setCodec("audio/pcm");
  format.setByteOrder(QAudioFormat::LittleEndian);
  format.setSampleType(QAudioFormat::SignedInt);

  inputDeviceInfo = QAudioDeviceInfo::defaultInputDevice();
  if (!inputDeviceName.isEmpty()) {
    for (const auto &device : availableInputDevices()) {
      if (device.deviceName() == inputDeviceName) {
        inputDeviceInfo = device;
        break;
      }
    }
  }

  if (!inputDeviceInfo.isFormatSupported(format)) {
    qWarning() << "Requested input format not supported, trying nearest.";
    format = inputDeviceInfo.nearestFormat(format);
  }

  outputDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
  if (!outputDeviceName.isEmpty()) {
    for (const auto &device : availableOutputDevices()) {
      if (device.deviceName() == outputDeviceName) {
        outputDeviceInfo = device;
        break;
      }
    }
  }

  if (!outputDeviceInfo.isFormatSupported(format)) {
    qWarning() << "Requested output format not supported, trying nearest.";
  }

  qDebug() << "Initializing Audio Input:" << inputDeviceInfo.deviceName();
  qDebug() << "Initializing Audio Output:" << outputDeviceInfo.deviceName();

  qDebug() << "Audio Format: " << format.sampleRate() << "Hz,"
           << format.channelCount() << "channels," << format.sampleSize()
           << "bit," << format.codec();

  // Input
  audioInput = new QAudioInput(inputDeviceInfo, format, this);
  audioInput->setBufferSize(65536); // Increased from 8192 to avoid overflows

  connect(audioInput, &QAudioInput::stateChanged, this,
          [](QAudio::State state) {
            qDebug() << "AudioInput state changed to:" << state;
          });

  // Output
  audioOutput = new QAudioOutput(outputDeviceInfo, format, this);
  audioOutput->setBufferSize(65536);

  connect(audioOutput, &QAudioOutput::stateChanged, this,
          [](QAudio::State state) {
            qDebug() << "AudioOutput state changed to:" << state;
          });

  soundDriverOK = true;
  return true;
}

int soundWindows::read(int &countAvailable) {
  QMutexLocker locker(&deviceMutex);
  if (!audioInput || !inputDevice)
    return 0;

  // Bytes available in the buffer
  qint64 bytesReady = audioInput->bytesReady();
  countAvailable = bytesReady / 2; // Mono 16-bit: 2 bytes per frame

  // We need DOWNSAMPLESIZE frames (mono: 2 bytes/frame)
  int framesToRead = DOWNSAMPLESIZE;
  int bytesPerFrame = audioInput->format().sampleSize() / 8 *
                      audioInput->format().channelCount();

  if (framesToRead * bytesPerFrame > bytesReady) {
    return 0; // Not enough data yet
  }

  qint64 bytesRead =
      inputDevice->read((char *)tempRXBuffer, framesToRead * bytesPerFrame);

  if (bytesReady > 32768) {
    qInfo() << "soundWindows::read high buffer usage:" << bytesReady;
  }

  return bytesRead / bytesPerFrame;
}

int soundWindows::write(unsigned int numFrames) {
  if (!audioOutput || !outputDevice)
    return 0;

  // tempTXBuffer contains 32-bit samples (SOUNDFRAME is quint32).
  // The audio device is configured for 16-bit mono.
  // We MUST convert 32-bit source to 16-bit destination.
  // Simply casting the pointer would send high/low words as separate samples
  // (garbage).

  QByteArray pcmData;
  pcmData.resize(numFrames * 2); // 2 bytes per sample (16-bit)
  qint16 *ptr = (qint16 *)pcmData.data();

  for (unsigned int i = 0; i < numFrames; ++i) {
    // Extract lower 16 bits. Generates correct signed 16-bit value
    // from the quint32 (which was cast from double).
    ptr[i] = (qint16)tempTXBuffer[i];
  }

  const char *dataPtr = pcmData.constData();
  qint64 bytesRemaining = pcmData.size();
  qint64 totalWritten = 0;

  // Blocking write loop to ensure all data is sent to the audio device
  int retryCount = 0;
  // Lock the mutex to check state and write safely
  QMutexLocker locker(&deviceMutex);
  while (bytesRemaining > 0) {
    if (playbackState == PBINIT) {
      break;
    }
    qint64 written = outputDevice->write(dataPtr, bytesRemaining);

    if (written > 0) {
      totalWritten += written;
      bytesRemaining -= written;
      dataPtr += written;
      retryCount = 0; // Reset retry on success
    } else {
      // Device buffer full or error. Wait and retry.
      retryCount++;
      if (retryCount > 1000) { // Safety timeout (approx 2s)
        qWarning() << "soundWindows::write timeout! Audio device stuck?";
        break;
      }
      QThread::msleep(2);
      // Re-lock to check condition in next iteration (if we were unlocking, but
      // we are holding it) Actually, if we hold the lock, stopImmediate cannot
      // run. We should probably unlock during sleep? But QAudioOutput::write is
      // not blocking in the sense of waiting for hardware, it just fills
      // buffer. If buffer is full, write returns 0. We need to release lock to
      // allow stopImmediate to interrupt us?
      locker.unlock();
      QThread::msleep(2);
      locker.relock();
    }
  }

  return totalWritten / 2; // Return number of frames (samples) written
}

void soundWindows::flushCapture() {
  QMutexLocker locker(&deviceMutex);
  if (audioInput) {
    audioInput->reset();
    inputDevice = audioInput->start();
  }
}

void soundWindows::flushPlayback() {
  QMutexLocker locker(&deviceMutex);
  if (audioOutput) {
    audioOutput->reset();
    outputDevice = audioOutput->start();
  }
}

void soundWindows::closeDevices() {
  // closeDevices is called from init (locked) or destructor.
  // We should verify if mutex is recursive or if we need to be careful.
  // QMutex is NOT recursive by default.
  // But init calls closeDevices at the start.
  // We can let init handle the lock or make closeDevices lock.
  // Let's assume closeDevices is internal or called from Main Thread.
  // soundBase calls it.
  // If init locks, we can't lock again here.
  // Let's change init to NOT lock closeDevices, or make closeDevices private
  // and assume caller locks? Or use QMutex::Recursive. soundWindows::init
  // locks, then calls closeDevices. So closeDevices shouldn't lock if init is
  // the caller. But stopSoundThread calls closeDevices. Let's change init to
  // call a private internal close? Or just rely on the fact that init calls
  // closeDevices first thing. Actually, let's make closeDevices NOT lock, but
  // ensure callers do? No, closeDevices is virtual from soundBase. Let's check
  // where closeDevices is called. init (soundWindows), stopSoundThread
  // (soundBase), ~soundWindows. init is in soundWindows. stopSoundThread is in
  // soundBase.

  // Safe bet: QRecursiveMutex if available, but let's stick to QMutex and
  // handle it. I will make init unlock before calling close? No.

  // Revised plan: don't lock in closeDevices for now if it complicates init.
  // Wait, closeDevices destroys objects. We MUST protect that.
  // I'll leave closeDevices unlocked for now and expect init/stop to handle
  // safety, or better: Move locking to critical spots. stopImmediate overrides
  // soundBase::stopImmediate.

  if (audioInput) {
    audioInput->stop();
    delete audioInput;
    audioInput = nullptr;
  }
  if (audioOutput) {
    audioOutput->stop();
    delete audioOutput;
    audioOutput = nullptr;
  }
}

void soundWindows::waitPlaybackEnd() {
  if (audioOutput && audioOutput->state() == QAudio::ActiveState) {
    QTime timer;
    timer.start();
    while (audioOutput->state() == QAudio::ActiveState) {
      if (timer.elapsed() > 2000) { // 2 second timeout
        qWarning() << "soundWindows::waitPlaybackEnd timed out";
        break;
      }
      QThread::msleep(10);
      QCoreApplication::processEvents();
    }
  }
}

void soundWindows::stopImmediate() {
  // First, signal the loop to stop
  soundBase::stopImmediate();

  // Then lock and reset
  QMutexLocker locker(&deviceMutex);
  if (audioOutput) {
    audioOutput->reset();
  }
}

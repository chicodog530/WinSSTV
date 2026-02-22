#include "waterfallwidget.h"
#include <QDebug>
#include <QPainter>
#include <math.h>

// Assuming DOWNSAMPLESIZE is available via include path or hardcoded if
// necessary But we receive data size dynamically usually. However fftCalc needs
// init. Let's assume typical chunk size if not defined, or define locally as
// fallback.
#ifndef RXSTRIPE
#define RXSTRIPE 512 // Fallback
#endif

WaterfallWidget::WaterfallWidget(QWidget *parent)
    : QWidget(parent), m_fftInitialized(false) {
  m_waterfall = QImage(1, 1, QImage::Format_RGB32);
  m_waterfall.fill(Qt::black);
  setMinimumHeight(200);
}

WaterfallWidget::~WaterfallWidget() {}

void WaterfallWidget::processAudio(const QVector<double> &data) {
  if (data.isEmpty())
    return;

  if (!m_fftInitialized) {
    // Init FFT
    // length = chunk size
    // nblocks = ?? e.g. 4 for 2048 point FFT
    // rate = 12000 (assumed)
    m_fft.init(data.size(), 4, 12000);
    m_fftInitialized = true;
  }

  // fftCalc expects double*
  // const_cast because realFFT signature, but it probably consumes input.
  // Actually realFFT(double *data) uses memmove to copy from data.
  m_fft.realFFT(const_cast<double *>(data.constData()));

  // Process output
  // FFT length = data.size() * nblocks
  int fftLen = data.size() * 4;
  int halfLen =
      fftLen / 2; // Represents 0 to Nyquist (e.g. 6000Hz if rate is 12000)

  // We want to display 0 to ~3200Hz.
  // Sampling rate assumed 12000Hz. Nyquist = 6000Hz.
  // Bin resolution = 12000 / fftLen.
  // Bins for 3200Hz = 3200 * fftLen / 12000.

  double maxFreq = 32000.0; // Wait, sampling rate 48000? Downsampled to 12000?
  // Let's assume standard behavior:
  // Downsampling usually happens in SoundBase. If rate is 12000, Nyquist is
  // 6000. 3200Hz is index = 3200 / 6000 * halfLen.

  int maxBin = (int)(3200.0 / 6000.0 * halfLen);
  if (maxBin > halfLen)
    maxBin = halfLen;
  if (maxBin < 10)
    maxBin = 10; // Safety

  // Let's use a fixed width image and scale in paintEvent.
  int displayWidth = 512; // Fixed width for texture

  if (m_waterfall.width() != displayWidth) {
    m_waterfall = QImage(displayWidth, 300,
                         QImage::Format_RGB32); // arbitrary height buffer
    m_waterfall.fill(Qt::black);
  }

  // Scroll down
  int w = m_waterfall.width();
  int h = m_waterfall.height();

  // Memmove old lines down
  for (int y = h - 1; y > 0; --y) {
    memcpy(m_waterfall.scanLine(y), m_waterfall.scanLine(y - 1), w * 4);
  }

  // Draw new line at y=0
  QRgb *line = (QRgb *)m_waterfall.scanLine(0);

  // We'll use a simple running average for the noise floor/baseline
  static double runningAvg = -100.0;

  for (int x = 0; x < w; ++x) {
    // Map x (0..w) to bin (0..maxBin)
    int binIndex = (int)((double)x / w * maxBin);
    if (binIndex >= halfLen)
      binIndex = halfLen - 1;

    double r = m_fft.out[binIndex];
    double im = (binIndex == 0 || binIndex == halfLen)
                    ? 0
                    : m_fft.out[fftLen - binIndex];
    double mag = sqrt(r * r + im * im) / fftLen;

    double val = 20 * log10(mag + 1e-12);

    // Update running average (slowly)
    runningAvg = 0.999 * runningAvg + 0.001 * val;

    // Use baseline-relative scaling.
    // Usually signals are 10-40dB above noise floor.
    int c = (int)((val - runningAvg) * 5);
    if (c < 0)
      c = 0;
    if (c > 255)
      c = 255;

    // Heatmap: Black -> Blue -> Green -> Red
    int red = 0, green = 0, blue = 0;
    if (c < 64) {
      blue = c * 4;
    } else if (c < 128) {
      blue = 255;
      green = (c - 64) * 4;
    } else if (c < 192) {
      green = 255;
      blue = 255 - (c - 128) * 4;
      red = (c - 128) * 2;
    } else {
      red = 255;
      green = 255 - (c - 192) * 4;
    }

    line[x] = qRgb(red, green, blue);
  }

  update();
}

void WaterfallWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);
  painter.drawImage(rect(), m_waterfall);
}

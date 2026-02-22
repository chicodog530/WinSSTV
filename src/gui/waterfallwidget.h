#ifndef WATERFALLWIDGET_H
#define WATERFALLWIDGET_H

#include "utils/fftcalc.h"
#include <QImage>
#include <QVector>
#include <QWidget>


class WaterfallWidget : public QWidget {
  Q_OBJECT
public:
  explicit WaterfallWidget(QWidget *parent = nullptr);
  ~WaterfallWidget();

public slots:
  void processAudio(const QVector<double> &data);

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  QImage m_waterfall;
  fftCalc m_fft;
  bool m_fftInitialized;
  int m_width;
  int m_height;
};

#endif // WATERFALLWIDGET_H

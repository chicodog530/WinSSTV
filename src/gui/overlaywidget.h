#ifndef OVERLAYWIDGET_H
#define OVERLAYWIDGET_H

#include <QImage>
#include <QList>
#include <QMouseEvent>
#include <QPoint>
#include <QRect>
#include <QString>
#include <QWidget>


struct TextOverlay {
  QString text;
  QPoint position;
  QColor color;
  int fontSize;
  QRect cachedRect; // For hit detection
};

class OverlayWidget : public QWidget {
  Q_OBJECT
public:
  explicit OverlayWidget(QWidget *parent = nullptr);

  void setImage(const QImage &image);
  void addTextOverlay(void) = delete; // Prevent accidental use
  void addTextOverlay(const QString &text, const QPoint &pos,
                      const QColor &color = Qt::yellow, int fontSize = 24);
  void clearOverlays();

  QImage getFinalImage(); // Returns image with overlays burned in

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;

private:
  QImage m_image;
  QList<TextOverlay> m_overlays;

  int m_draggingIndex = -1;
  QPoint m_dragStartPos;
  QPoint m_overlayStartPos;
};

#endif // OVERLAYWIDGET_H

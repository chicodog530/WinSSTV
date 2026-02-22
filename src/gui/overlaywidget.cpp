#include "overlaywidget.h"
#include <QDebug>
#include <QPainter>

OverlayWidget::OverlayWidget(QWidget *parent) : QWidget(parent) {
  // Enable mouse tracking if needed for hover effects
  // setMouseTracking(true);
}

void OverlayWidget::setImage(const QImage &image) {
  m_image = image;
  update(); // Trigger repaint
}

void OverlayWidget::addTextOverlay(const QString &text, const QPoint &pos,
                                   const QColor &color, int fontSize) {
  TextOverlay overlay;
  overlay.text = text;
  overlay.position = pos;
  overlay.color = color;
  overlay.fontSize = fontSize;
  m_overlays.append(overlay);
  update();
}

void OverlayWidget::clearOverlays() {
  m_overlays.clear();
  update();
}

QImage OverlayWidget::getFinalImage() {
  if (m_image.isNull())
    return QImage();

  QImage result = m_image.copy();
  QPainter painter(&result);

  for (const auto &overlay : m_overlays) {
    painter.setPen(overlay.color);
    QFont font = painter.font();
    font.setPointSize(overlay.fontSize);
    painter.setFont(font);
    painter.drawText(overlay.position, overlay.text);
  }

  return result;
}

void OverlayWidget::paintEvent(QPaintEvent *event) {
  Q_UNUSED(event);
  QPainter painter(this);

  if (m_image.isNull()) {
    painter.fillRect(rect(), Qt::lightGray);
    painter.drawText(rect(), Qt::AlignCenter, "No Image Loaded");
    return;
  }

  // Scale image to fit widget while maintaining aspect ratio
  QImage scaled =
      m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);

  // Calculate position to center the image
  int x = (width() - scaled.width()) / 2;
  int y = (height() - scaled.height()) / 2;

  painter.drawImage(x, y, scaled);

  double scaleFactor = (double)scaled.width() / m_image.width();

  // Iterate by reference to update cachedRect
  for (auto &overlay : m_overlays) {
    painter.setPen(overlay.color);
    QFont font = painter.font();
    // Scale font size visually
    int scaledFontSize = static_cast<int>(overlay.fontSize * scaleFactor);
    font.setPointSize(scaledFontSize > 0 ? scaledFontSize : 1);
    painter.setFont(font);

    // Transform position (Image coords -> Widget coords)
    QPoint displayPos(x + overlay.position.x() * scaleFactor,
                      y + overlay.position.y() * scaleFactor);

    // Calculate bounding rect for hit detection
    QFontMetrics fm(font);
    QRect boundingRect = fm.boundingRect(overlay.text);
    // Adjust rect to position
    boundingRect.moveTopLeft(displayPos);
    // Store in widget coordinates
    overlay.cachedRect = boundingRect;

    painter.drawText(displayPos, overlay.text);

    // Optional: Draw box if dragging this one?
    // if (&overlay == &m_overlays[m_draggingIndex]) ...
  }
}

void OverlayWidget::mousePressEvent(QMouseEvent *event) {
  m_draggingIndex = -1;
  // Check in reverse order to grab top-most overlay first
  for (int i = m_overlays.size() - 1; i >= 0; --i) {
    if (m_overlays[i].cachedRect.contains(event->pos())) {
      m_draggingIndex = i;
      m_dragStartPos = event->pos();
      m_overlayStartPos = m_overlays[i].position;
      // Set cursor to indicate drag
      setCursor(Qt::SizeAllCursor);
      return;
    }
  }
  QWidget::mousePressEvent(event);
}

void OverlayWidget::mouseMoveEvent(QMouseEvent *event) {
  if (m_draggingIndex != -1 && m_draggingIndex < m_overlays.size()) {
    QPoint delta = event->pos() - m_dragStartPos;

    // Recalculate scale factor to convert delta back to image coordinates
    // We can't easily cache this without risky state, so re-calculate implies
    // we assume the image hasn't changed size during drag (safe assumption).
    QImage scaled =
        m_image.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    double scaleFactor = (double)scaled.width() / m_image.width();

    if (scaleFactor > 0) {
      // specific delta in image coords
      int dx = static_cast<int>(delta.x() / scaleFactor);
      int dy = static_cast<int>(delta.y() / scaleFactor);

      m_overlays[m_draggingIndex].position = m_overlayStartPos + QPoint(dx, dy);
      update(); // Repaint
    }
  } else {
    QWidget::mouseMoveEvent(event);
  }
}

void OverlayWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (m_draggingIndex != -1) {
    m_draggingIndex = -1;
    setCursor(Qt::ArrowCursor);
  }
  QWidget::mouseReleaseEvent(event);
}

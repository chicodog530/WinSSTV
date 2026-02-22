#ifndef IMAGEVIEWER_H
#define IMAGEVIEWER_H

#include <QImage>
#include <QObject>
#include <QString>


// Minimal stub to satisfy sstvTx
// We will replace this with a clean interface later.

class imageViewer : public QObject {
  Q_OBJECT
public:
  explicit imageViewer(QObject *parent = 0) : QObject(parent) {}

  QImage *getImagePtr() { return &img; }
  QString getFilename() { return "stub_image.png"; }
  QString getCompressedFilename() { return "stub_compressed.bin"; }
  int getFileSize() { return 0; }

  // Fields accessed directly?
  QString toCall;
  QString toOperator;
  QString rsv;

private:
  QImage img;
};

#endif // IMAGEVIEWER_H

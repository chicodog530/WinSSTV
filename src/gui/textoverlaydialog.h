#ifndef TEXTOVERLAYDIALOG_H
#define TEXTOVERLAYDIALOG_H

#include <QColor>
#include <QDialog>


class QLineEdit;
class QPushButton;
class QSpinBox;
class QLabel;

class TextOverlayDialog : public QDialog {
  Q_OBJECT

public:
  explicit TextOverlayDialog(QWidget *parent = nullptr,
                             const QString &initialText = "",
                             const QColor &initialColor = Qt::yellow,
                             int initialSize = 24);

  QString getText() const;
  QColor getColor() const;
  int getFontSize() const;

private slots:
  void chooseColor();

private:
  QLineEdit *m_textEdit;
  QPushButton *m_colorButton;
  QSpinBox *m_sizeSpinBox;
  QLabel *m_previewLabel;

  QColor m_color;

  void updatePreview();
};

#endif // TEXTOVERLAYDIALOG_H

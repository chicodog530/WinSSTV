#include "textoverlaydialog.h"
#include <QColorDialog>
#include <QDebug>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>

TextOverlayDialog::TextOverlayDialog(QWidget *parent,
                                     const QString &initialText,
                                     const QColor &initialColor,
                                     int initialSize)
    : QDialog(parent), m_color(initialColor) {
  setWindowTitle(tr("Text Overlay Settings (v3)")); // Versioned title
  setMinimumSize(400, 300);                         // Force visible size

  qDebug() << "TextOverlayDialog::ctor - initialText:" << initialText
           << " Color:" << initialColor << " Size:" << initialSize;

  QVBoxLayout *layout = new QVBoxLayout(this);

  // Text Input
  m_textEdit = new QLineEdit(this);
  m_textEdit->setText(initialText); // Explicitly set text
  qDebug() << "TextOverlayDialog init with text:" << initialText;
  m_textEdit->setPlaceholderText(tr("Enter text to display"));
  layout->addWidget(new QLabel(tr("Text:"), this));
  layout->addWidget(m_textEdit);

  QHBoxLayout *settingsLayout = new QHBoxLayout();

  // Font Size
  m_sizeSpinBox = new QSpinBox(this);
  m_sizeSpinBox->setRange(8, 500); // 72 is too small for hi-res images
  m_sizeSpinBox->setValue(initialSize > 0 ? initialSize
                                          : 48); // Default to larger size
  settingsLayout->addWidget(new QLabel(tr("Font Size (pt):"), this));
  settingsLayout->addWidget(m_sizeSpinBox);

  // Color Button
  m_colorButton = new QPushButton(tr("Change Color"), this);
  // Set button background to current color
  QString qss = QString("background-color: %1; color: %2;")
                    .arg(m_color.name())
                    .arg(m_color.lightness() > 128 ? "black" : "white");
  m_colorButton->setStyleSheet(qss);

  connect(m_colorButton, &QPushButton::clicked, this,
          &TextOverlayDialog::chooseColor);
  settingsLayout->addWidget(m_colorButton);

  layout->addLayout(settingsLayout);

  // Preview Label
  m_previewLabel = new QLabel(tr("Preview"), this);
  m_previewLabel->setAlignment(Qt::AlignCenter);
  m_previewLabel->setStyleSheet("background-color: #333;");
  m_previewLabel->setFixedHeight(100);
  layout->addWidget(m_previewLabel);

  // Buttons
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  QPushButton *okButton = new QPushButton(tr("OK"), this);
  QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);

  connect(okButton, &QPushButton::clicked, this, &QDialog::accept);
  connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

  buttonLayout->addWidget(okButton);
  buttonLayout->addWidget(cancelButton);
  layout->addLayout(buttonLayout);

  // Connect changes to update preview
  connect(m_textEdit, &QLineEdit::textChanged, this,
          &TextOverlayDialog::updatePreview);
  connect(m_sizeSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this,
          &TextOverlayDialog::updatePreview);

  updatePreview();
}

void TextOverlayDialog::chooseColor() {
  QColor color = QColorDialog::getColor(m_color, this, tr("Select Text Color"));
  if (color.isValid()) {
    m_color = color;
    QString qss = QString("background-color: %1; color: %2;")
                      .arg(m_color.name())
                      .arg(m_color.lightness() > 128 ? "black" : "white");
    m_colorButton->setStyleSheet(qss);
    updatePreview();
  }
}

QString TextOverlayDialog::getText() const { return m_textEdit->text(); }

QColor TextOverlayDialog::getColor() const { return m_color; }

int TextOverlayDialog::getFontSize() const { return m_sizeSpinBox->value(); }

void TextOverlayDialog::updatePreview() {
  QString text = m_textEdit->text();
  if (text.isEmpty())
    text = tr("Preview Text");

  m_previewLabel->setText(text);

  QString qss = QString("background-color: #333; color: %1; font-size: %2pt; "
                        "font-weight: bold;")
                    .arg(m_color.name())
                    .arg(m_sizeSpinBox->value());
  m_previewLabel->setStyleSheet(qss);
}

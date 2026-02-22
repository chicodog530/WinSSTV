#ifndef RIGCONTROL_H
#define RIGCONTROL_H

#include <QObject>
#include <QString>
#include <hamlib/rig.h>

class RigControl : public QObject {
  Q_OBJECT
public:
  explicit RigControl(QObject *parent = nullptr);
  ~RigControl();

  bool open();
  void close();
  bool isOpen() const;

  double getFreq();
  bool setFreq(double freq);

  bool setPTT(ptt_t ptt);
  ptt_t getPTT();

signals:
  void frequencyChanged(double freq);
  void catStatusChanged(bool ok, QString message);

private:
  RIG *m_rig;
  bool m_isOpen;
};

#endif // RIGCONTROL_H

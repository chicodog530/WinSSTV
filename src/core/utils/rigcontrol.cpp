#include "rigcontrol.h"
#include "appglobal.h"
#include <QDebug>
#include <QSerialPortInfo>
#include <hamlib/port.h>
#include <hamlib/rig.h>
#include <hamlib/rig_state.h>

RigControl::RigControl(QObject *parent)
    : QObject(parent), m_rig(nullptr), m_isOpen(false) {}

RigControl::~RigControl() { close(); }

bool RigControl::open() {
  if (m_isOpen)
    close();

  qDebug() << "Opening Rig Model:" << catRigModel << "on port" << catSerialPort;

  m_rig = rig_init(catRigModel);
  if (!m_rig) {
    emit catStatusChanged(
        false, tr("Failed to initialize rig model %1").arg(catRigModel));
    return false;
  }

  hamlib_port_t *rp = HAMLIB_RIGPORT(m_rig);
  if (rp) {
    rp->type.rig = RIG_PORT_SERIAL;
    strncpy(rp->pathname, catSerialPort.toLatin1().data(),
            HAMLIB_FILPATHLEN - 1);
    rp->parm.serial.rate = catBaudRate;
  }

  // Set PTT method if specified
  if (catPTTMethod != RIG_PTT_NONE) {
    hamlib_port_t *pp = HAMLIB_PTTPORT(m_rig);
    if (pp) {
      pp->type.ptt = (ptt_type_t)catPTTMethod;
    }
  }

  qDebug() << "Calling rig_open...";
  int ret = rig_open(m_rig);
  qDebug() << "rig_open returned:" << ret;
  if (ret != RIG_OK) {
    emit catStatusChanged(false,
                          tr("Failed to open rig: %1").arg(rigerror(ret)));
    rig_cleanup(m_rig);
    m_rig = nullptr;
    return false;
  }

  m_isOpen = true;
  emit catStatusChanged(true, tr("Rig opened successfully"));
  return true;
}

void RigControl::close() {
  if (m_rig) {
    rig_close(m_rig);
    rig_cleanup(m_rig);
    m_rig = nullptr;
  }
  m_isOpen = false;
}

bool RigControl::isOpen() const { return m_isOpen; }

double RigControl::getFreq() {
  if (!m_isOpen)
    return 0;
  freq_t freq;
  int ret = rig_get_freq(m_rig, RIG_VFO_CURR, &freq);
  if (ret != RIG_OK) {
    qDebug() << "rig_get_freq error:" << rigerror(ret);
    return 0;
  }
  return (double)freq;
}

bool RigControl::setFreq(double freq) {
  if (!m_isOpen)
    return false;
  int ret = rig_set_freq(m_rig, RIG_VFO_CURR, (freq_t)freq);
  if (ret != RIG_OK) {
    qDebug() << "rig_set_freq error:" << rigerror(ret);
    return false;
  }
  emit frequencyChanged(freq);
  return true;
}

bool RigControl::setPTT(ptt_t ptt) {
  if (!m_isOpen)
    return false;
  int ret = rig_set_ptt(m_rig, RIG_VFO_CURR, ptt);
  if (ret != RIG_OK) {
    qDebug() << "rig_set_ptt error:" << rigerror(ret);
    return false;
  }
  return true;
}

ptt_t RigControl::getPTT() {
  if (!m_isOpen)
    return RIG_PTT_OFF;
  ptt_t ptt;
  int ret = rig_get_ptt(m_rig, RIG_VFO_CURR, &ptt);
  if (ret != RIG_OK) {
    qDebug() << "rig_get_ptt error:" << rigerror(ret);
    return RIG_PTT_OFF;
  }
  return ptt;
}

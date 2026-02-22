#ifndef CONFIGPARAMS_H
#define CONFIGPARAMS_H

#include <QString>

// Stub global config variables used by hybridCrypt and others
extern QString hybridFtpRemoteHost;
extern QString hybridFtpLogin;
extern QString hybridFtpPassword;
extern QString hybridFtpRemoteDirectory;
extern int soundRoutingInput;
extern int soundRoutingOutput;
extern QString inputDeviceName;
extern QString outputDeviceName;
extern int recordingSize;
extern int cwWPM;
extern int cwTone;
extern bool useCW;
extern QString cwText;
extern bool useVOX;

#include <QColor>
extern QColor imageBackGroundColor;
extern bool pttToneOtherChannel;
extern bool swapChannel;

#endif // CONFIGPARAMS_H

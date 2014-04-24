#ifndef TEST_GUI_KCONNECTTESTCLIENT_H
#define TEST_GUI_KCONNECTTESTCLIENT_H

#include <QObject>
#include <QSet>
#include <QtQml/QQmlParserStatus>

#include <QtCrypto>

#include "daemon.h"
#include "device.h"

namespace test {
namespace gui {

class KConnectTestClient : public QObject, public QQmlParserStatus
{
    Q_OBJECT

    Q_PROPERTY(Daemon* kconnectDaemon READ kconnectDaemon WRITE setKconnectDaemon NOTIFY kconnectDaemonChanged)

    Q_INTERFACES(QQmlParserStatus)

public:
    explicit KConnectTestClient(QObject *parent = 0);

    // QQmlParserStatus interface
    virtual void classBegin();
    virtual void componentComplete();

    Daemon* kconnectDaemon() const;

public slots:
    void setKconnectDaemon(Daemon* daemon);
    void onAcceptPairing();

private slots:
    void onDeviceAdded(const QString& id);

signals:
    void kconnectDaemonChanged(Daemon* daemon);
    void pairNotification(const QString& deviceName);

private:
    // The Initializer object sets things up, and also does cleanup when it goes out of scope
    QCA::Initializer mQcaInitializer;

    Daemon* m_kconnectDaemon = nullptr;
    Device* m_device = nullptr;
};

} // namespace gui
} // namespace test

#endif // TEST_GUI_KCONNECTTESTCLIENT_H

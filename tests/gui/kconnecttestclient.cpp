#include "kconnecttestclient.h"

#include <QVariant>
#include <QSettings>
#include <QUuid>

#include <QDebug>

namespace test {
namespace gui {

KConnectTestClient::KConnectTestClient(QObject *parent) :
    QObject(parent)
{
}

void KConnectTestClient::classBegin()
{
}

void KConnectTestClient::componentComplete()
{
    QSettings settings;
    settings.beginGroup("myself");

    if(!settings.contains("id")) {
        QString uuid = QUuid::createUuid().toString();
        settings.setValue("id", uuid);
    }

    if(!QCA::isSupported("pkey") ||
            !QCA::PKey::supportedIOTypes().contains(QCA::PKey::RSA)) {
        //TODO: Maybe display this in a more visible way?
        qWarning() << "Error: KDE Connect could not find support for RSA in your QCA installation, if your distribution provides"
                   << "separate packages for QCA-ossl and QCA-gnupg plugins, make sure you have them installed and try again";
        return;
    }

    if(!settings.contains("privateKey") || !settings.contains("publicKey")) {
        QCA::PrivateKey privateKey = QCA::KeyGenerator().createRSA(2048);
        settings.setValue("privateKey", privateKey.toPEM());

        QCA::PublicKey publicKey = privateKey.toPublicKey();
        settings.setValue("publicKey", publicKey.toPEM());
    }

    settings.endGroup();

    Daemon* daemon = new Daemon(this);

    connect(daemon, SIGNAL(deviceAdded(const QString&)), this, SLOT(onDeviceAdded(const QString&)));

    setKconnectDaemon(daemon);
}

Daemon *KConnectTestClient::kconnectDaemon() const
{
    return m_kconnectDaemon;
}

void KConnectTestClient::setKconnectDaemon(Daemon *daemon)
{
    if (m_kconnectDaemon != daemon) {
        m_kconnectDaemon = daemon;
        emit kconnectDaemonChanged(daemon);
    }
}

void KConnectTestClient::onAcceptPairing()
{
    qDebug() << "Accept Pairing";
    m_device->acceptPairing();
}

void KConnectTestClient::onDeviceAdded(const QString &id)
{
    m_device = m_kconnectDaemon->device(id);

    connect(m_device, SIGNAL(pairNotification(QString)), SIGNAL(pairNotification(QString)));
}

} // namespace gui
} // namespace test

/**
 * Copyright 2013 Albert Vaca <albertvaka@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "daemon.h"

#include <QVariant>
#include <QSettings>
#include <QUuid>
//#include <QDBusConnection>
#include <QNetworkSession>
#include <QNetworkConfigurationManager>

#include <QDebug>

#include "networkpackage.h"
#include "backends/lan/lanlinkprovider.h"
//#include "backends/loopback/loopbacklinkprovider.h"

//K_PLUGIN_FACTORY(KdeConnectFactory, registerPlugin<Daemon>();)
//K_EXPORT_PLUGIN(KdeConnectFactory("kdeconnect", "kdeconnect"))

Daemon::Daemon(QObject *parent)
    : QObject(parent)
{
    //Debugging
    qDebug() << "Starting KdeConnect daemon";

    //Load backends (hardcoded by now, should be plugins in a future)
    mLinkProviders.insert(new LanLinkProvider());
    //mLinkProviders.insert(new LoopbackLinkProvider());

    //Read remebered paired devices
    QSettings settings;
    settings.beginGroup("trusted_devices");
    const QStringList& list = settings.childKeys();
    Q_FOREACH(const QString& id, list) {
        Device* device = new Device(id);
        connect(device, SIGNAL(reachableStatusChanged()),
                this, SLOT(onDeviceReachableStatusChanged()));
        mDevices[id] = device;
        emit deviceAdded(id);
    }
    settings.endGroup();

    //Listen to connectivity changes
    QNetworkSession* network = new QNetworkSession(QNetworkConfigurationManager().defaultConfiguration());
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        connect(network, SIGNAL(stateChanged(QNetworkSession::State)),
                a, SLOT(onNetworkChange(QNetworkSession::State)));
        connect(a, SIGNAL(onConnectionReceived(NetworkPackage, DeviceLink*)),
                this, SLOT(onNewDeviceLink(NetworkPackage, DeviceLink*)));
    }

    //QDBusConnection::sessionBus().registerService("org.kde.kdeconnect");
    //QDBusConnection::sessionBus().registerObject("/modules/kdeconnect", this, QDBusConnection::ExportScriptableContents);

    setDiscoveryEnabled(true);

}

void Daemon::setDiscoveryEnabled(bool b)
{
    //Listen to incomming connections
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        if (b)
            a->onStart();
        else
            a->onStop();
    }

}

void Daemon::forceOnNetworkChange()
{
    Q_FOREACH (LinkProvider* a, mLinkProviders) {
        a->onNetworkChange(QNetworkSession::Connected);
    }
}

QStringList Daemon::visibleDevices()
{
    QStringList ret;
    Q_FOREACH(Device* device, mDevices) {
        if (device->isReachable()) {
            ret.append(device->id());
        }
    }
    return ret;
}

Device *Daemon::device(const QString &id)
{
    return mDevices.value(id);
}

QStringList Daemon::devices()
{
    return mDevices.keys();
}

void Daemon::onNewDeviceLink(const NetworkPackage& identityPackage, DeviceLink* dl)
{

    const QString& id = identityPackage.get<QString>("deviceId");

    qDebug() << "Device discovered" << id << "via" << dl->provider()->name();

    if (mDevices.contains(id)) {
        qDebug() << "It is a known device";
        Device* device = mDevices[id];
        device->addLink(identityPackage, dl);
    } else {
        qDebug() << "It is a new device";

        Device* device = new Device(identityPackage, dl);
        connect(device, SIGNAL(reachableStatusChanged()), this, SLOT(onDeviceReachableStatusChanged()));
        mDevices[id] = device;

        emit deviceAdded(id);
    }

    emit deviceVisibilityChanged(id, true);

}

void Daemon::onDeviceReachableStatusChanged()
{

    Device* device = (Device*)sender();
    QString id = device->id();

    emit deviceVisibilityChanged(id, device->isReachable());

    qDebug() << "Device" << device->name() << "reachable status changed:" << device->isReachable();

    if (!device->isReachable()) {

        if (!device->isPaired()) {
            qDebug() << "Destroying device" << device->name();
            emit deviceRemoved(id);
            mDevices.remove(id);
            device->deleteLater();
        }

    }

}

Daemon::~Daemon()
{

}


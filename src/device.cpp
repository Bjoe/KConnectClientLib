#include "device.h"

#include <QVariant>
#include <QSettings>

#include <QDebug>

//#include "plugins/pluginloader.h"
#include "backends/devicelink.h"
#include "backends/linkprovider.h"
#include "networkpackage.h"

Device::Device(const QString& id)
    : m_deviceId(id)
    , m_pairStatus(Device::Paired)
    , m_protocolVersion(NetworkPackage::ProtocolVersion) //We don't know it yet
{
    QSettings settings;
    settings.beginGroup("trusted_devices");
    settings.beginGroup(id);

    m_deviceName = settings.value("deviceName", QLatin1String("unnamed")).toString();
    m_deviceType = str2type(settings.value("deviceType", QLatin1String("unknown")).toString());
    const QString& key = settings.value("publicKey", QString()).toString();
    m_publicKey = QCA::RSAPublicKey::fromPEM(key);

    settings.endGroup();
    settings.endGroup();

    //Register in bus
    //QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);

}

Device::Device(const NetworkPackage& identityPackage, DeviceLink* dl)
    : m_deviceId(identityPackage.get<QString>("deviceId"))
    , m_deviceName(identityPackage.get<QString>("deviceName"))
    , m_deviceType(str2type(identityPackage.get<QString>("deviceType")))
    , m_pairStatus(Device::NotPaired)
    , m_protocolVersion(identityPackage.get<int>("protocolVersion"))
{
    addLink(identityPackage, dl);

    //Register in bus
    //QDBusConnection::sessionBus().registerObject(dbusPath(), this, QDBusConnection::ExportScriptableContents | QDBusConnection::ExportAdaptors);
}

Device::~Device()
{

}

bool Device::hasPlugin(const QString& name) const
{
    return m_plugins.contains(name);
}

QStringList Device::loadedPlugins() const
{
    return m_plugins.keys();
}

void Device::reloadPlugins()
{
/*    QMap<QString, KdeConnectPlugin*> newPluginMap;
    QMultiMap<QString, KdeConnectPlugin*> newPluginsByInterface;

    if (isPaired() && isReachable()) { //Do not load any plugin for unpaired devices, nor useless loading them for unreachable devices

        QString path = KGlobal::dirs()->findResource("config", "kdeconnect/"+id());
        KConfigGroup pluginStates = KSharedConfig::openConfig(path)->group("Plugins");

        PluginLoader* loader = PluginLoader::instance();

        //Code borrowed from KWin
        foreach (const QString& pluginName, loader->getPluginList()) {
            QString enabledKey = pluginName + QString::fromLatin1("Enabled");

            bool isPluginEnabled = (pluginStates.hasKey(enabledKey) ? pluginStates.readEntry(enabledKey, false)
                                                            : loader->getPluginInfo(pluginName).isPluginEnabledByDefault());

            if (isPluginEnabled) {
                KdeConnectPlugin* plugin = m_plugins.take(pluginName);
                QStringList interfaces;
                if (plugin) {
                    interfaces = m_pluginsByinterface.keys(plugin);
                } else {
                    PluginData data = loader->instantiatePluginForDevice(pluginName, this);
                    plugin = data.plugin;
                    interfaces = data.interfaces;
                }
                foreach(const QString& interface, interfaces) {
                    newPluginsByInterface.insert(interface, plugin);
                }
                newPluginMap[pluginName] = plugin;
            }
        }
    }

    //Erase all left plugins in the original map (meaning that we don't want
    //them anymore, otherwise they would have been moved to the newPluginMap)
    qDeleteAll(m_plugins);
    m_plugins = newPluginMap;
    m_pluginsByinterface = newPluginsByInterface;

    Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
        plugin->connected();
    }

    Q_EMIT pluginsChanged();
*/
}

void Device::requestPair()
{
    switch(m_pairStatus) {
        case Device::Paired:
            emit pairingFailed("Already paired");
            return;
        case Device::Requested:
            emit pairingFailed("Pairing already requested for this device");
            return;
        default:
            if (!isReachable()) {
                emit pairingFailed("Device not reachable");
                return;
            }
            break;
    }

    m_pairStatus = Device::Requested;

    //Send our own public key
    bool success = sendOwnPublicKey();

    if (!success) {
        m_pairStatus = Device::NotPaired;
        emit pairingFailed("Error contacting device");
        return;
    }

    if (m_pairStatus == Device::Paired) return;
    pairingTimer.setSingleShot(true);
    pairingTimer.start(30 * 1000);
    connect(&pairingTimer, SIGNAL(timeout()),
            this, SLOT(pairingTimeout()));

}

void Device::unpair()
{
    if (!isPaired()) return;

    m_pairStatus = Device::NotPaired;

    QSettings settings;
    settings.beginGroup("trusted_devices");
    settings.remove(id());
    settings.endGroup();

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);

    reloadPlugins(); //Will unload the plugins

    Q_EMIT unpaired();

}

void Device::pairingTimeout()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);
    m_pairStatus = Device::NotPaired;
    emit pairingFailed("Timed out");
}

static bool lessThan(DeviceLink* p1, DeviceLink* p2)
{
    return p1->provider()->priority() > p2->provider()->priority();
}

void Device::addLink(const NetworkPackage& identityPackage, DeviceLink* link)
{
    //qDebug() << "Adding link to" << id() << "via" << link->provider();

    m_protocolVersion = identityPackage.get<int>("protocolVersion");
    if (m_protocolVersion != NetworkPackage::ProtocolVersion) {
        qWarning() << m_deviceName << "- warning, device uses a different protocol version" << m_protocolVersion << "expected" << NetworkPackage::ProtocolVersion;
    }

    connect(link, SIGNAL(destroyed(QObject*)),
            this, SLOT(linkDestroyed(QObject*)));

    m_deviceLinks.append(link);

    //re-read the device name from the identityPackage because it could have changed
    m_deviceName = identityPackage.get<QString>("deviceName");
    m_deviceType = str2type(identityPackage.get<QString>("deviceType"));

    //TODO: Do not read the key every time from config, store somewhere
    QSettings settings;
    settings.beginGroup("myself");
    const QString& key = settings.value("privateKey", QString()).toString();
    QCA::PrivateKey privateKey = QCA::PrivateKey::fromPEM(key);
    link->setPrivateKey(privateKey);

    //Theoretically we will never add two links from the same provider (the provider should destroy
    //the old one before this is called), so we do not have to worry about destroying old links.
    //Actually, we should not destroy them or the provider will store an invalid ref!

    connect(link, SIGNAL(receivedPackage(NetworkPackage)),
            this, SLOT(privateReceivedPackage(NetworkPackage)));

    qSort(m_deviceLinks.begin(), m_deviceLinks.end(), lessThan);

    if (m_deviceLinks.size() == 1) {
        reloadPlugins(); //Will load the plugins
        Q_EMIT reachableStatusChanged();
    } else {

        // Plugins werden Aktivirt ??!

        /*
        Q_FOREACH(KdeConnectPlugin* plugin, m_plugins) {
            plugin->connected();
        }
        */
    }
}

void Device::linkDestroyed(QObject* o)
{
    removeLink(static_cast<DeviceLink*>(o));
}

void Device::removeLink(DeviceLink* link)
{
    m_deviceLinks.removeOne(link);

    //qDebug() << "RemoveLink" << m_deviceLinks.size() << "links remaining";

    if (m_deviceLinks.isEmpty()) {
        reloadPlugins();
        Q_EMIT reachableStatusChanged();
    }
}

bool Device::sendPackage(NetworkPackage& np)
{
    if (np.type() != PACKAGE_TYPE_PAIR && isPaired()) {
        Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
            if (dl->sendPackageEncrypted(m_publicKey, np)) return true;
        }
    } else {
        //Maybe we could block here any package that is not an identity or a pairing package to prevent sending non encrypted data
        Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
            if (dl->sendPackage(np)) return true;
        }
    }

    return false;
}

void Device::privateReceivedPackage(const NetworkPackage& np)
{
    if (np.type() == PACKAGE_TYPE_PAIR) {

        //qDebug() << "Pair package";

        bool wantsPair = np.get<bool>("pair");

        if (wantsPair == isPaired()) {
            qDebug() << "Already" << (wantsPair? "paired":"unpaired");
            if (m_pairStatus == Device::Requested) {
                m_pairStatus = Device::NotPaired;
                pairingTimer.stop();
                emit pairingFailed("Canceled by other peer");
            }
            return;
        }

        if (wantsPair) {

            //Retrieve their public key
            const QString& key = np.get<QString>("publicKey");
            m_publicKey = QCA::RSAPublicKey::fromPEM(key);
            if (m_publicKey.isNull()) {
                qDebug() << "ERROR decoding key";
                if (m_pairStatus == Device::Requested) {
                    m_pairStatus = Device::NotPaired;
                    pairingTimer.stop();
                }
                emit pairingFailed("Received incorrect key");
                return;
            }

            if (m_pairStatus == Device::Requested)  { //We started pairing

                qDebug() << "Pair answer";
                setAsPaired();

            } else {

                qDebug() << "Pair request";

                emit pairNotification(m_deviceName);
                /*
                KNotification* notification = new KNotification("pingReceived"); //KNotification::Persistent
                notification->setPixmap(KIcon("dialog-information").pixmap(48, 48));
                notification->setComponentData(KComponentData("kdeconnect", "kdeconnect"));
                notification->setTitle("KDE Connect");
                notification->setText(i18n("Pairing request from %1", m_deviceName));
                notification->setActions(QStringList() << i18n("Accept") << i18n("Reject"));
                */
                //connect(notification, SIGNAL(action1Activated()), this, SLOT(acceptPairing()));
                //connect(notification, SIGNAL(action2Activated()), this, SLOT(rejectPairing()));
                //notification->sendEvent();

                m_pairStatus = Device::RequestedByPeer;

            }

        } else {

            qDebug() << "Unpair request";

            if (m_pairStatus == Device::Requested) {
                pairingTimer.stop();
                emit pairingFailed("Canceled by other peer");
            } else if (m_pairStatus == Device::Paired) {
                QSettings settings;
                settings.beginGroup("trusted_devices");
                settings.remove(id());
                settings.endGroup();
                reloadPlugins();
                emit unpaired();
            }

            m_pairStatus = Device::NotPaired;

        }

    } else if (isPaired()) {

        // Plugin bekommt das Packet

        /*
        QList<KdeConnectPlugin*> plugins = m_pluginsByinterface.values(np.type());
        foreach(KdeConnectPlugin* plugin, plugins) {
            plugin->receivePackage(np);
        }
        */
    } else {
        //TODO: Notify the other side that we don't trust them
        qDebug() << "device" << name() << "not paired, ignoring package" << np.type();
    }

}

bool Device::sendOwnPublicKey()
{
    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", true);
    QSettings settings;
    settings.beginGroup("myself");
    const QString& key = settings.value("publicKey", QString()).toString();
    settings.endGroup();
    np.set("publicKey",key);
    bool success = sendPackage(np);
    return success;
}

void Device::rejectPairing()
{
    qDebug() << "Rejected pairing";

    m_pairStatus = Device::NotPaired;

    NetworkPackage np(PACKAGE_TYPE_PAIR);
    np.set("pair", false);
    sendPackage(np);

    emit pairingFailed("Canceled by the user");

}

void Device::acceptPairing()
{
    if (m_pairStatus != Device::RequestedByPeer) return;

    qDebug() << "Accepted pairing";

    bool success = sendOwnPublicKey();

    if (!success) {
        m_pairStatus = Device::NotPaired;
        return;
    }

    setAsPaired();

}

void Device::setAsPaired()
{

    m_pairStatus = Device::Paired;

    pairingTimer.stop(); //Just in case it was started

    storeAsTrusted(); //Save to the config as trusted

    reloadPlugins(); //Will actually load the plugins

    Q_EMIT pairingSuccesful();

}

void Device::storeAsTrusted()
{
    QSettings settings;
    settings.beginGroup("trusted_devices");
    settings.beginGroup(id());

    settings.setValue("publicKey", m_publicKey.toPEM());
    settings.setValue("deviceName", name());
    settings.setValue("deviceType", type2str(m_deviceType));

    settings.endGroup();
    settings.endGroup();
}

QStringList Device::availableLinks() const
{
    QStringList sl;
    Q_FOREACH(DeviceLink* dl, m_deviceLinks) {
        sl.append(dl->provider()->name());
    }
    return sl;
}

void Device::sendPing()
{
    NetworkPackage np(PACKAGE_TYPE_PING);
    bool success = sendPackage(np);
    qDebug() << "sendPing:" << success;
}

Device::DeviceType Device::str2type(QString deviceType) {
    if (deviceType == "desktop") return Desktop;
    if (deviceType == "laptop") return Laptop;
    if (deviceType == "phone") return Phone;
    if (deviceType == "tablet") return Tablet;
    return Unknown;
}

QString Device::type2str(Device::DeviceType deviceType) {
    if (deviceType == Desktop) return "desktop";
    if (deviceType == Laptop) return "laptop";
    if (deviceType == Phone) return "phone";
    if (deviceType == Tablet) return "tablet";
    return "unknown";
}

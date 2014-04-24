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

#include "networkpackage.h"

#include <QByteArray>
#include <QDataStream>
#include <QHostInfo>
#include <QSslKey>
#include <QDateTime>
#include <QtCrypto>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "filetransferjob.h"

const QCA::EncryptionAlgorithm NetworkPackage::EncryptionAlgorithm = QCA::EME_PKCS1v15;
const int NetworkPackage::ProtocolVersion = 5;

NetworkPackage::NetworkPackage(const QString& type)
{
    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = type;
    mBody = QVariantMap();
    mPayload = QSharedPointer<QIODevice>();
    mPayloadSize = 0;
}

void NetworkPackage::createIdentityPackage(NetworkPackage* np)
{
    //KSharedConfigPtr config = KSharedConfig::openConfig("kdeconnectrc");
    QString id = "emulator";//config->group("myself").readEntry<QString>("id","");
    np->mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    np->mType = PACKAGE_TYPE_IDENTITY;
    np->mPayload = QSharedPointer<QIODevice>();
    np->mPayloadSize = 0;
    np->set("deviceId", id);
    np->set("deviceName", QHostInfo::localHostName());
    np->set("protocolType", "desktop"); //TODO: Detect laptop, tablet, phone...
    np->set("protocolVersion",  NetworkPackage::ProtocolVersion);

    qDebug() << "createIdentityPackage" << np->serialize();
}

QByteArray NetworkPackage::serialize() const
{
    QJsonObject jsonObject;
    jsonObject["id"] = mId;
    jsonObject["type"] = mType;
    jsonObject["body"] = QJsonObject::fromVariantMap(mBody);

    if (hasPayload()) {
        qDebug() << "Serializing payloadTransferInfo";
        jsonObject["payloadSize"] = 0;
        jsonObject["payloadTransferInfo"] = QJsonObject::fromVariantMap(mPayloadTransferInfo);
    }

    jsonObject["version"] = NetworkPackage::ProtocolVersion;

    QJsonDocument jsonSerializer(jsonObject);

    QByteArray json = jsonSerializer.toJson();
    return json;
}

bool NetworkPackage::unserialize(const QByteArray& data, NetworkPackage* networkPackage)
{
    QJsonDocument jsonSerializer = QJsonDocument::fromJson(data);
    QJsonObject jsonObject = jsonSerializer.object();

    QJsonValue idValue = jsonObject["id"];
    networkPackage->mId = idValue.toString();

    QJsonValue typeValue = jsonObject["type"];
    networkPackage->mType = typeValue.toString();

    QJsonObject bodyValue = jsonObject.value("body").toObject();
    networkPackage->mBody = bodyValue.toVariantMap();

    QJsonValue payloadSizeValue = jsonObject["payloadSize"];
    if(payloadSizeValue.type() != QJsonValue::Undefined) {
        networkPackage->mPayloadSize = payloadSizeValue.toInt();
    } else {
        networkPackage->mPayloadSize = 0;
    }

    QJsonValue payloadTransferInfoValue = jsonObject["payloadTransferInfo"];
    if(payloadTransferInfoValue.type() != QJsonValue::Undefined) {
        QJsonObject payloadTransferInfo = payloadTransferInfoValue.toObject();
        networkPackage->mPayloadTransferInfo = payloadTransferInfo.toVariantMap();
    } else {
        networkPackage->mPayloadTransferInfo = QVariantMap();
    }

    return true;
}

void NetworkPackage::encrypt(QCA::PublicKey& key)
{

    QByteArray serialized = serialize();

    int chunkSize = key.maximumEncryptSize(NetworkPackage::EncryptionAlgorithm);

    QStringList chunks;
    while (!serialized.isEmpty()) {
        QByteArray chunk = serialized.left(chunkSize);
        serialized = serialized.mid(chunkSize);
        QByteArray encryptedChunk = key.encrypt(chunk, NetworkPackage::EncryptionAlgorithm).toByteArray();
        chunks.append( encryptedChunk.toBase64() );
    }

    qDebug() << chunks.size() << "chunks";

    mId = QString::number(QDateTime::currentMSecsSinceEpoch());
    mType = PACKAGE_TYPE_ENCRYPTED;
    mBody = QVariantMap();
    mBody["data"] = chunks;

}

bool NetworkPackage::decrypt(QCA::PrivateKey& key, NetworkPackage* out) const
{

    const QStringList& chunks = mBody["data"].toStringList();

    QByteArray decryptedJson;
    Q_FOREACH(const QString& chunk, chunks) {
        QByteArray encryptedChunk = QByteArray::fromBase64(chunk.toLatin1());
        QCA::SecureArray decryptedChunk;
        bool success = key.decrypt(encryptedChunk, &decryptedChunk, NetworkPackage::EncryptionAlgorithm);
        if (!success) {
            return false;
        }
        decryptedJson.append(decryptedChunk.toByteArray());
    }

    bool success = unserialize(decryptedJson, out);

    if (!success) return false;

    if (hasPayload()) {
        out->mPayload = mPayload;
    }

    return true;

}

FileTransferJob* NetworkPackage::createPayloadTransferJob(const QUrl &destination) const
{
    return new FileTransferJob(payload(), payloadSize(), destination);
}


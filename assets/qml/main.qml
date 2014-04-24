import QtQuick 2.1
import QtQuick.Controls 1.0
import QtQuick.Window 2.0
import QtQuick.Layouts 1.0
import KConnect 1.0

ApplicationWindow {
    id: mainWindow
    title: qsTr("KDE Connect Client Emulator")
    width: 360
    height: 360

    menuBar: MenuBar {
        Menu {
            title: qsTr("Datei")

            MenuItem {
                text: qsTr("Exit")
                onTriggered: Qt.quit();
            }
        }
    }

    statusBar: StatusBar {
        id: statusBar
        RowLayout {
            ProgressBar {
                id: statusBarProgress
            }
            Label {
                id: statusBarMessage
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent

            Label {
                Layout.fillWidth: true
                Layout.fillHeight: true
                id: deviceStatus
                text: "status"
            }

            Button {
                id: requestButton
                enabled: false
                text: qsTr("Ok")
                onClicked: kconnectClient.onAcceptPairing();
            }
    }

    KConnectClient {
        id: kconnectClient

        onKconnectDaemonChanged: {
            daemon.deviceAdded.connect(deviceAdded)
            daemon.deviceRemoved.connect(deviceRemoved)
            daemon.deviceVisibilityChanged.connect(deviceVisibilityChanged)
        }

        onPairNotification: {
            statusBarMessage.text = "Device " + deviceName + " added.";
            requestButton.enabled = true;
        }
    }

    function deviceAdded(id)
    {
        statusBarMessage.text = "Device " + id + " added.";
    }

    function deviceRemoved(id)
    {
        statusBarMessage.text = "Device " + id + " removed.";
    }

    function deviceVisibilityChanged(id, isVisible)
    {
        statusBarMessage.text = "Device " + id + " is visible: " + isVisible;
    }
}

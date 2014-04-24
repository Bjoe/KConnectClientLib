
#include <stdio.h>
#include <stdlib.h>

#include <QString>
#include <QByteArray>
#include <QUrl>
#include <QtGui/QGuiApplication>

#include <QtMessageHandler>
#include <QMessageLogContext>

#include <QDebug>

//#include <QtCrypto>

#include "testconfig.h"
#include "gui/mainwindowuicontroller.h"

void messageHandlerOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();
    switch (type) {
    case QtDebugMsg:
        fprintf(stderr, "Debug: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtWarningMsg:
        fprintf(stderr, "Warning: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s (%s:%u, %s)\n", localMsg.constData(), context.file, context.line, context.function);
        abort();
    }
}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(messageHandlerOutput);

    // The Initializer object sets things up, and also
    // does cleanup when it goes out of scope
    //QCA::Initializer init;

    QGuiApplication app(argc, argv);
    app.setApplicationName("kdeconnectlib");
    app.setApplicationDisplayName("kconnect");
    app.setOrganizationName("devolab");

    test::gui::MainWindowUiController* window = test::gui::MainWindowUiController::createInstance(QML_PATH);
    window->show();

    return app.exec();
}

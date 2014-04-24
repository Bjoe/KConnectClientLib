#include "mainwindowuicontroller.h"

#include <qqml.h>

#include "kconnecttestclient.h"

namespace test {
namespace gui {

MainWindowUiController *MainWindowUiController::createInstance(const QString &qmlUrl)
{
    qmlRegisterType<test::gui::KConnectTestClient>("KConnect", 1, 0, "KConnectClient");

    QQmlApplicationEngine* engine = new QQmlApplicationEngine();
    initContext(engine);
    engine->load(QUrl(qmlUrl));
    QQuickWindow* window = createAndInitWindow(engine);
    return new MainWindowUiController(engine, window);
}

void MainWindowUiController::initContext(QQmlApplicationEngine* engine)
{
    QQmlContext* context = engine->rootContext();

}

QQuickWindow* MainWindowUiController::createAndInitWindow(QQmlApplicationEngine* engine)
{
    QList<QObject *> list = engine->rootObjects();
    QObject *topLevel = list.value(0);
    QQuickWindow* window = qobject_cast<QQuickWindow *>(topLevel);

    return window;
}

void MainWindowUiController::show()
{
    m_window->show();
}

MainWindowUiController::MainWindowUiController(QQmlApplicationEngine *engine, QQuickWindow *window)
    : m_engine(engine), m_window(window)
{
}

MainWindowUiController::~MainWindowUiController()
{
    delete m_engine;
    delete m_window;
}

} // namespace gui
} // namespace kccl

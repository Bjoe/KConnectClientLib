#ifndef TEST_GUI_MAINWINDOWUICONTROLLER_H
#define TEST_GUI_MAINWINDOWUICONTROLLER_H

#include <QString>
#include <QtQml/QQmlApplicationEngine>
#include <QtQuick/QQuickWindow>

namespace test {
namespace gui {

class MainWindowUiController
{
public:
    ~MainWindowUiController();

    static MainWindowUiController *createInstance(const QString &qmlUrl);
    void show();

private:
    static void initContext(QQmlApplicationEngine *engine);
    static QQuickWindow *createAndInitWindow(QQmlApplicationEngine *engine);
    MainWindowUiController(QQmlApplicationEngine* engine, QQuickWindow* window);

    QQmlApplicationEngine* m_engine;
    QQuickWindow* m_window;
};

} // namespace gui
} // namespace test

#endif // TEST_GUI_MAINWINDOWUICONTROLLER_H

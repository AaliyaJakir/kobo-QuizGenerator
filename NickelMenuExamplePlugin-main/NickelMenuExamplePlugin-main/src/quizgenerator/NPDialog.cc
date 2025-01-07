#include <QApplication>
#include <QComboBox>
#include <QRect>
#include <QScreen>
#include <QTouchEvent>

#include "NPDialog.h"

const char touchFilterSet[] = "touchFilterSet";

NPDialog::NPDialog(QWidget *parent) : QDialog(parent)
{
    setAttribute(Qt::WA_AcceptTouchEvents);
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect geom = screen->availableGeometry();
    
    // Set to full screen size
    setGeometry(0, 0, geom.width(), geom.height());
    
    // Make sure dialog stays on top and covers the full screen
    setWindowFlags(Qt::Dialog | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint);
    setWindowState(Qt::WindowFullScreen);
}

bool NPDialog::eventFilter(QObject *obj, QEvent *event)
{
    auto type = event->type();
    if (type == QEvent::TouchBegin || type == QEvent::TouchUpdate || type == QEvent::TouchEnd) {
        event->accept();
        auto tp = static_cast<QTouchEvent*>(event)->touchPoints().at(0);
        if (type == QEvent::TouchBegin) {
            QMouseEvent md(QEvent::MouseButtonPress, tp.pos(), tp.screenPos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(obj, &md);
        } else if (type == QEvent::TouchUpdate) {
            QMouseEvent mm(QEvent::MouseMove, tp.pos(), tp.screenPos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(obj, &mm);
        } else if (type == QEvent::TouchEnd) {
            QMouseEvent mu(QEvent::MouseButtonRelease, tp.pos(), tp.screenPos(), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
            QApplication::sendEvent(obj, &mu);
        }
        return true;
    }
    return false;
}

void NPDialog::installEvFilter(QWidget *w)
{
    if (!w->property(touchFilterSet).isValid()) {
        w->setProperty(touchFilterSet, QVariant(true));
        w->setAttribute(Qt::WA_AcceptTouchEvents);
        w->installEventFilter(this);
    }
}

void NPDialog::showDlg()
{
    auto children = findChildren<QWidget*>();
    for (int i = 0; i < children.size(); ++i) {
        if (auto wi = children.at(i)) {
            installEvFilter(wi);
        }
    }
    open();
}
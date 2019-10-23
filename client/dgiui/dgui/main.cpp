#include <QStyle>
#include <QDesktopWidget>
#include "dgui.h"
#include <QtWidgets/QApplication>
#include "view/auth/WgtAuth.h"
#include "logic/DgiEngine.h"




int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dgui* mainWindow = new dgui();

    if (dguard::GetEngine().isPasswordSet())
    {
        view::WgtAuth* auth = new view::WgtAuth();

        QObject::connect(auth, SIGNAL(signalLogged()), mainWindow, SLOT(handlerStart()));

        auth->show();
        auth->move(qApp->desktop()->screen()->rect().center() - auth->rect().center());
        auth->setFixedSize(auth->geometry().width(), auth->geometry().height());
    }
    else
    {
        view::WgtAuthSetPass* setPassword = new view::WgtAuthSetPass();

        QObject::connect(setPassword, SIGNAL(signalPasswordSet(std::wstring)), mainWindow, SLOT(handlerPasswordSet(std::wstring)));

        setPassword->show();
        setPassword->move(qApp->desktop()->screen()->rect().center() - setPassword->rect().center());
        setPassword->setFixedSize(setPassword->geometry().width(), setPassword->geometry().height());
    }

    return a.exec();
}

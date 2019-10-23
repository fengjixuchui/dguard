/********************************************************************************
** Form generated from reading UI file 'dgui.ui'
**
** Created by: Qt User Interface Compiler version 5.13.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_DGUI_H
#define UI_DGUI_H

#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QStatusBar>
#include <QtWidgets/QToolBar>
#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE

class Ui_dguiClass
{
public:
    QMenuBar *menuBar;
    QToolBar *mainToolBar;
    QWidget *centralWidget;
    QStatusBar *statusBar;

    void setupUi(QMainWindow *dguiClass)
    {
        if (dguiClass->objectName().isEmpty())
            dguiClass->setObjectName(QString::fromUtf8("dguiClass"));
        dguiClass->resize(600, 400);
        menuBar = new QMenuBar(dguiClass);
        menuBar->setObjectName(QString::fromUtf8("menuBar"));
        dguiClass->setMenuBar(menuBar);
        mainToolBar = new QToolBar(dguiClass);
        mainToolBar->setObjectName(QString::fromUtf8("mainToolBar"));
        dguiClass->addToolBar(mainToolBar);
        centralWidget = new QWidget(dguiClass);
        centralWidget->setObjectName(QString::fromUtf8("centralWidget"));
        dguiClass->setCentralWidget(centralWidget);
        statusBar = new QStatusBar(dguiClass);
        statusBar->setObjectName(QString::fromUtf8("statusBar"));
        dguiClass->setStatusBar(statusBar);

        retranslateUi(dguiClass);

        QMetaObject::connectSlotsByName(dguiClass);
    } // setupUi

    void retranslateUi(QMainWindow *dguiClass)
    {
        dguiClass->setWindowTitle(QCoreApplication::translate("dguiClass", "dgui", nullptr));
    } // retranslateUi

};

namespace Ui {
    class dguiClass: public Ui_dguiClass {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_DGUI_H

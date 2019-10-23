//
//	Author:
//			Burlutsky Stanislav
//			burluckij@gmail.com
//	
//  Copyright (C) Burlutsky Stanislav (burluckij@gmail.com). All Rights Reserved.
//

#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QGroupBox>
#include <QTreeWidget>
#include <QProgressBar>
#include <QStylePainter>
#include <QSystemTrayIcon>
#include <QTreeWidget.h>
#include <QTableWidgetItem>
#include <qmenu.h>
#include <qmenubar.h>

#include "../ExtendedTree.h"

#define TXT_ACT_ADD_FILE               QObject::tr("Add file")
#define TXT_ACT_ADD_DIR                QObject::tr("Add directory")


namespace view
{
    class WgtFLockContextAddNew : public QWidget
    {
        Q_OBJECT

    private:
        QMenu* m_contextMenu;
        QAction* m_actAddFile;
        QAction* m_actAddDir;

    public:
        WgtFLockContextAddNew()
        {
            m_contextMenu = new QMenu(this);

            m_actAddFile = m_contextMenu->addAction(TXT_ACT_ADD_FILE);
            m_actAddDir = m_contextMenu->addAction(TXT_ACT_ADD_DIR);

            m_actAddDir->setIcon(QIcon(":dgui/ico/folder.ico"));
            m_actAddFile->setIcon(QIcon(":dgui/ico/file_24x24.ico"));

            //m_actLock->setIcon(UiResources::GetMe().icoOk());

            connect(m_contextMenu, SIGNAL(triggered(QAction*)), this, SLOT(slotActivated(QAction*)));
        }

        QMenu* Get()
        {
            return m_contextMenu;
        }

        //
        //  Qualifiers object's type and builds menu
        //

        void ShowMenu()
        {
            m_contextMenu->exec(QCursor::pos());
        }

    signals:
        void signalAddFile();
        void signalAddDir();


    public slots:

        void slotActivated(QAction* _pAction)
        {
            if (_pAction == m_actAddFile)
            {
                emit signalAddFile();
            }
            else if (_pAction == m_actAddDir)
            {
                emit signalAddDir();
            }
        }
    };
}


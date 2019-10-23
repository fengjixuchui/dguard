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

#include "../ExtendedTree.h"
#include "WgtFLockContextAddNew.h"
#include "WgtFLockContextChange.h"
#include "../logic/DgiCommon.h"

namespace view
{
	class WgtFLock : public QWidget
    {
		Q_OBJECT

	public:

		WgtFLock(QWidget *parent = 0);

        void updateProtectedFiles();

    public slots:
        void addNewFlock();
        void deleteProtectedFiles();
        void handlerFLockClicked(QTreeWidgetItem* item, int column);
        void prepareMenu(const QPoint& pos);

        void handlerToLock(QTreeWidgetItem*);
        void handlerToHide(QTreeWidgetItem*);
        void handlerToLockAndHide(QTreeWidgetItem*);
        void handlerToUnlock(QTreeWidgetItem*);
        void handlerToRemove(QTreeWidgetItem*);

        void handlerAddFile();
        void handlerAddDir();

	private:

		QLabel* m_header;
		QLabel* m_description;
		QPushButton* m_add;
		QPushButton* m_removeAll;
		dguard::ExtendedTree* m_tree;
        view::WgtFLockContextChange* m_contextChangeState;
        view::WgtFLockContextAddNew* m_conextAdd;

    protected:
        void changeFlockState(std::string _flockId, ::dguard::FLockProtectionState _state);
	};

}

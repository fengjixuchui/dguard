
//      (c)VsoftLab 2006 - 2013
//		Author: burluckij@gmail.com	


#pragma once
#include <QObject>
#include <windows.h>
#include <QTreeWidget>
#include <QMouseEvent>

namespace dguard
{
	class ExtendedTree : public QTreeWidget
	{
		Q_OBJECT

	public:
		ExtendedTree(QWidget * parent = 0) :
            m_rightButtonClicked(false),
            QTreeWidget(parent){}

		bool rightButtonClicked() const
        {
			return m_rightButtonClicked;
		}

	protected:
		virtual void mousePressEvent(QMouseEvent* event) override
        {
			m_rightButtonClicked = (event->button() == Qt::MouseButton::RightButton);

//             if (!m_rightButtonClicked)
//             {
//                 QTreeWidget::mousePressEvent(event);
//             }

            QTreeWidget::mousePressEvent(event);
		}

	private:
		volatile bool m_rightButtonClicked;
	};

}

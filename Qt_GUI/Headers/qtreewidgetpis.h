#pragma once

#include <QTreeWidget>
#include <QDropEvent>

/// <summary>
/// The QTreeWidget class is inherited for the Raspberry Pis to override the dropEvent method and to at a signal
/// </summary>
class QTreeWidgetPis : public QTreeWidget {

    Q_OBJECT

    // Use the base constructor
    using QTreeWidget::QTreeWidget;

protected:
    /// <summary>
    /// Called when a drop event occurs over the widget (Project of a Raspberry Pi changed)
    /// </summary>
    /// <param name="event">The event</param>
    void dropEvent(QDropEvent* event);

signals:
    /// <summary>
    /// Signals when the dropEvent occured over the widget (Project of a Raspberry Pi changed)
    /// </summary>
    /// <param name="item">The item where the dropEvent occured</param>
    void itemDropped(QTreeWidgetItem* item);

};

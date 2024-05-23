#include "qtreewidgetpis.h"

void QTreeWidgetPis::dropEvent(QDropEvent* event) {
    // Save the item where the dropEvent occured
    QTreeWidgetItem* item = itemAt(event->pos());

    // Expand the project item to see the Raspberry item(s)
    expandItem(item);

    // Statically call the dropEvent method of QTreeWidget
    QTreeWidget::dropEvent(event);

    // Signal that the dropEvent occured
    itemDropped(item);

    // De-select all items
    clearSelection();

    // Center the item where the dropEvent occured
    scrollToItem(item, QAbstractItemView::PositionAtCenter);
}


from PyQt4.QtCore import *
from PyQt4.QtGui import *

class CursorLabel(QLabel):
    def __init__(self, parent=None):
        super(CursorLabel, self).__init__(parent)
        self.cursor = 0

    def paintEvent(self, event):
        QLabel.paintEvent(self, event)
        painter = QPainter(self)
        pen = QPen(QColor(255,0,0, 128))
        pen.setWidth(3)
        painter.setPen(pen)
        painter.drawLine(QPoint(self.cursor, 0),
                QPoint(self.cursor, self.height()))
        painter.end()


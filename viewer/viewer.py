#!/usr/bin/env python

import sys, os, time
from PyQt4.QtCore import *
from PyQt4.QtGui import *
import ui_viewer

class ViewerWindow(QMainWindow):
    def __init__(self, parent=None):
        super(ViewerWindow, self).__init__(parent)
        self.ui = ui_viewer.Ui_MainWindow()
        self.ui.setupUi(self)
        self.timer = QTimer(self)
        self.connect(self.timer, SIGNAL("timeout()"), self.update)
        self.image = QImage()
        self.time = 0
        self.lpos = 0
        self.paused = False
        self.interval = 100
        self.ui.scrollArea.setWidget(self.ui.imageLabel)
        #self.ui.scrollArea.setWidgetResizable(True)
        self.connect(self.ui.browseButton, SIGNAL("clicked()"), self.browse)
        self.connect(self.ui.playButton, SIGNAL("clicked()"), self.play)
        self.connect(self.ui.stopButton, SIGNAL("clicked()"), self.stop)

    def update(self):
        # center on pos
        self.time += self.interval
        pos = int(self.time/self.len*self.image.width())
        if pos > self.image.width():
            pos = 0
            self.stop()
        self.ui.imageLabel.cursor = pos
        #print "---"
        #print "cas:", self.time/1000.0, "s", "/", self.len/1000.0
        #print "pozice:", pos, "/", self.image.width()
        self.ui.progress.setValue(self.time)
        self.ui.imageLabel.update()
        if self.ui.centeredCheck.isChecked():
            margin = self.ui.scrollArea.width()//2-10
            self.ui.scrollArea.ensureVisible(pos,self.image.height()//2, margin)
        else:
            width = self.ui.scrollArea.width()
            margin = 3*width/4.0
            if pos - self.lpos > margin:
                self.ui.scrollArea.ensureVisible(pos+margin,
                        self.image.height()//2, width-margin)
                self.lpos = pos

    def browse(self):
        filename = QFileDialog.getOpenFileName(self, "Choose spectrogram",
                ".", "Images (*.png *.jpg *.bmp *.xpm);;All files (*.*)")
        if not filename:
            return
        self.ui.pathEdit.setText(filename)
        self.image = QImage(filename)
        if self.image.isNull():
            QMessageBox.information(self, "error", "Couldn't read that file")
            return
        self.ui.imageLabel.setPixmap(QPixmap.fromImage(self.image))
        self.ui.imageLabel.adjustSize()

    def play(self):
        if self.paused:
            self.timer.start(self.interval)
            self.paused = False
            return
        if self.timer.isActive():
            self.timer.stop()
            self.paused = True
            return

        if self.image.isNull():
            return
        cmd = str(self.ui.commandEdit.text())
        if self.ui.commandCheck.isChecked() and str:
            os.system(cmd)
        #for i in xrange(0,5):
        #    print i
        #    time.sleep(1)
        pps = self.ui.ppsSpin.value()
        self.interval = 40
        self.len = self.image.width()*1000.0/pps
        self.ui.progress.setMaximum(int(self.len))
        if not self.timer.isActive():
            self.timer.start(self.interval)

    def stop(self):
        self.timer.stop()
        self.time = 0
        self.paused = False

def main():
    app = QApplication(sys.argv)
    form = ViewerWindow()
    form.show()
    app.exec_()

if __name__ == "__main__":
    main()

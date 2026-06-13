# -*- coding: utf-8 -*-

################################################################################
## Form generated from reading UI file 'monitor_connect_zecu.ui'
##
## Created by: Qt User Interface Compiler version 6.6.3
##
## WARNING! All changes made in this file will be lost when recompiling UI file!
################################################################################

from PySide6.QtCore import (QCoreApplication, QDate, QDateTime, QLocale,
    QMetaObject, QObject, QPoint, QRect,
    QSize, QTime, QUrl, Qt)
from PySide6.QtGui import (QBrush, QColor, QConicalGradient, QCursor,
    QFont, QFontDatabase, QGradient, QIcon,
    QImage, QKeySequence, QLinearGradient, QPainter,
    QPalette, QPixmap, QRadialGradient, QTransform)
from PySide6.QtWidgets import (QAbstractButton, QApplication, QDialog, QDialogButtonBox,
    QHeaderView, QSizePolicy, QTreeView, QWidget)

class Ui_ConnectZECU(object):
    def setupUi(self, ConnectZECU):
        if not ConnectZECU.objectName():
            ConnectZECU.setObjectName(u"ConnectZECU")
        ConnectZECU.resize(761, 638)
        self.buttonBox = QDialogButtonBox(ConnectZECU)
        self.buttonBox.setObjectName(u"buttonBox")
        self.buttonBox.setGeometry(QRect(270, 600, 461, 32))
        self.buttonBox.setOrientation(Qt.Horizontal)
        self.buttonBox.setStandardButtons(QDialogButtonBox.Cancel|QDialogButtonBox.Ok)
        self.TreeView_ListECU = QTreeView(ConnectZECU)
        self.TreeView_ListECU.setObjectName(u"TreeView_ListECU")
        self.TreeView_ListECU.setGeometry(QRect(20, 20, 701, 551))

        self.retranslateUi(ConnectZECU)
        self.buttonBox.accepted.connect(ConnectZECU.accept)
        self.buttonBox.rejected.connect(ConnectZECU.reject)

        QMetaObject.connectSlotsByName(ConnectZECU)
    # setupUi

    def retranslateUi(self, ConnectZECU):
        ConnectZECU.setWindowTitle(QCoreApplication.translate("ConnectZECU", u"Dialog", None))
    # retranslateUi


#pragma once
#ifndef VMP_PCH
#define VMP_PCH

#include <QtCore/QMap> // because of non-usual new operator
#include <QtCore/QHash> // because of non-usual new operator
#include <QtCore/QMetaType> // because of non-usual new operator
#include <QtCore/QVector> // because of non-usual new operator
#include "../core/precompiled.h"

#include <QtWidgets/QtWidgets>
#include <QtWidgets/QDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QApplication>
#include <QtCore/QObject>
#include <QtWidgets/QAbstractScrollArea>
#include <QtWidgets/QAction>
#include <QtGui/QClipboard>
#include <QtGui/QPaintEvent>
#include <QtGui/QDrag>
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
#include <QtWidgets/QInputContext>
#endif
#include <QtCore/QMimeData>
#include <QtWidgets/QMenu>
#include <QtWidgets/QScrollBar>
#include <QtCore/QTimer>
#include <QtCore/QTextCodec>
#include <QtGui/QPainter>
#include <QtGui/QTextFormat>
#include <QtCore/QVarLengthArray>
#include <QtGui/QPaintDevice>
#include <QtGui/QFont>
#include <QtGui/QColor>
#include <QtCore/QRect>
#include <QtGui/QPaintEngine>
#include <QtWidgets/QWidget>
#include <QtGui/QPixmap>
#include <QtWidgets/QAction>
#include <QtCore/QTime>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QDesktopWidget>
#include <QtGui/QTextLayout>
#include <QtGui/QTextLine>
#include <QtCore/QLibrary>
#include <QtCore/QProcess>
#include <QtConcurrent/QtConcurrent>
#ifndef VMP_GNU
#include "QtWinExtras/qwinfunctions.h"
#endif
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTabBar>
#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpContentWidget>
#include <QtHelp/QHelpSearchQuery>
#include <QtHelp/QHelpSearchEngine>
#include <QtHelp/QHelpSearchResultWidget>

#include "../third-party/scintilla/Platform.h"
#include "../third-party/scintilla/Scintilla.h"
#include "../third-party/scintilla/ScintillaEditBase.h"
#include "../third-party/scintilla/SciLexer.h"
#include "../third-party/scintilla/ScintillaQt.h"
#include "../third-party/scintilla/PlatQt.h"

#endif //VMP_PCH
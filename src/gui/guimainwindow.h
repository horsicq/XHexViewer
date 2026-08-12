/* Copyright (c) 2019-2026 hors<horsicq@gmail.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#ifndef GUIMAINWINDOW_H
#define GUIMAINWINDOW_H

#include <QDragEnterEvent>
#include <QFileDialog>
#include <QMainWindow>
#include <QMimeData>
#include <QList>
#include <QMenu>

#include "../global.h"
#include "xhexviewwidget.h"
#include "dialogabout.h"
#include "dialogoptions.h"
#include "dialogshortcuts.h"
#include "xinfomenu.h"

namespace Ui {
class GuiMainWindow;
}

class XHexViewEx;
class QLabel;
class QComboBox;
class QLineEdit;

class GuiMainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit GuiMainWindow(QWidget *pParent = nullptr);
    ~GuiMainWindow() override;

private slots:
    void createMenus();
    void actionOpenSlot();
    void actionCloseSlot();
    void actionExitSlot();
    void actionShortcutsSlot();
    void actionOptionsSlot();
    void actionAboutSlot();
    void actionFileInformationSlot();
    void actionCopyFilePathSlot();
    void actionCopyFileNameSlot();
    void actionCopyFolderPathSlot();
    void actionShowInFolderSlot();
    void actionHexCommandSlot();
    void actionNavigateBackSlot();
    void actionNavigateForwardSlot();
    void actionGoToFileStartSlot();
    void actionGoToFileEndSlot();
    void actionQuickGoSlot();
    void actionElementModeSlot();
    void actionTextEncodingSlot();
    void actionBytesPerLineSlot();
    void actionLocationModeSlot();
    void actionLocationBaseSlot();
    void actionCopyChecksumSlot();
    void actionCopySelectionFormattedSlot();
    void actionCopySelectionRangeSlot();
    void actionCopySelectionSizeSlot();
    void actionZoomInSlot();
    void actionZoomOutSlot();
    void actionResetZoomSlot();
    void actionStayOnTopSlot(bool bChecked);
    void actionOverviewMapSlot(bool bChecked);
    void actionResetDisplaySlot();
    void selectionChangedSlot();
    void viewWidgetsStateChangedSlot();
    void updateRecentFilesState();
    void adjustWindow();
    void processFile(const QString &sFileName);
    void errorMessageSlot(const QString &sText);
    void closeCurrentFile();

protected:
    void dragEnterEvent(QDragEnterEvent *pEvent) override;
    void dragMoveEvent(QDragMoveEvent *pEvent) override;
    void dropEvent(QDropEvent *pEvent) override;

private:
    QAction *addHexAction(QMenu *pMenu, const QString &sText, const char *pMethod, XOptions::ICONTYPE iconType);
    QAction *addSelectionAction(QMenu *pMenu, const QString &sText, const char *pMethod, XOptions::ICONTYPE iconType);
    QAction *addViewAction(QMenu *pMenu, const QString &sText, const char *pMethod, XOptions::ICONTYPE iconType, qint32 nViewWidget);
    void addMenuShortcutHint(QAction *pAction, quint64 nShortcutId);
    void updateMenuShortcutHints();
    void updateFileStatus();
    void copyTextToClipboard(const QString &sText, const QString &sMessage);
    bool readSelectedBytes(QByteArray *pData, qint64 nMaximumSize = 1024 * 1024);
    void setDocumentActionsEnabled(bool bState);
    void updateQuickOffset();
    void setBytesPerLine(qint32 nBytesPerLine, bool bShowMessage = true);
    void setElementMode(XHexView::ELEMENT_MODE mode, bool bShowMessage = true);
    void setTextEncoding(const QString &sCodePage, bool bShowMessage = true);
    void setLocationMode(XBinaryView::LOCMODE mode, bool bShowMessage = true);
    void setLocationBase(qint32 nBase, bool bShowMessage = true);
    void changeZoom(qint32 nDelta);
    void syncDisplayActions();
    void updateNavigationActions();

    Ui::GuiMainWindow *ui;
    QAction *g_pActionClose;
    QAction *g_pActionNavigateBack;
    QAction *g_pActionNavigateForward;
    QAction *g_pActionOverviewMap;
    QAction *g_pActionStayOnTop;
    QLabel *g_pFileStatus;
    QComboBox *g_pBytesPerLineCombo;
    QComboBox *g_pElementModeCombo;
    QLineEdit *g_pQuickOffsetLineEdit;
    QList<QAction *> g_listDocumentActions;
    QList<QAction *> g_listSelectionActions;
    QList<QAction *> g_listViewActions;
    QList<QAction *> g_listBytesPerLineActions;
    QList<QAction *> g_listElementModeActions;
    QList<QAction *> g_listTextEncodingActions;
    QList<QAction *> g_listLocationModeActions;
    QList<QAction *> g_listLocationBaseActions;
    QList<QAction *> g_listShortcutHintActions;
    XInfoMenu *g_pInfoMenu;
    XOptions g_xOptions;
    XShortcuts g_xShortcuts;

    QFile *g_pFile;
    XInfoDB *g_pXInfo;
    XHexViewEx *g_pHexView;
    qint32 g_nBytesPerLine;
    XHexView::ELEMENT_MODE g_elementMode;
    QString g_sTextEncoding;
};

#endif  // GUIMAINWINDOW_H

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
#include "guimainwindow.h"

#include <QActionGroup>
#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFont>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QMdiSubWindow>
#include <QStatusBar>
#include <QSignalBlocker>
#include <QStringList>
#include <QToolBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

#include "ui_guimainwindow.h"
#include "xhexviewex.h"

GuiMainWindow::GuiMainWindow(QWidget *pParent) : QMainWindow(pParent), ui(new Ui::GuiMainWindow)
{
    ui->setupUi(this);

    ui->mdiArea->setViewMode(QMdiArea::SubWindowView);
    ui->mdiArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->mdiArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->labelWelcomeIcon->setPixmap(QIcon(QStringLiteral(":/main.png")).pixmap(QSize(64, 64)));
    ui->stackedWidget->setCurrentWidget(ui->pageWelcome);
    statusBar()->setObjectName(QStringLiteral("mainStatusBar"));
    statusBar()->setSizeGripEnabled(false);
    g_pFileStatus = new QLabel(tr("Ready - open or drop a binary file"), statusBar());
    g_pFileStatus->setObjectName(QStringLiteral("fileStatusLabel"));
    statusBar()->addPermanentWidget(g_pFileStatus);

    g_pFile = nullptr;
    g_pXInfo = nullptr;
    g_pHexView = nullptr;
    g_pActionClose = nullptr;
    g_pActionNavigateBack = nullptr;
    g_pActionNavigateForward = nullptr;
    g_pActionOverviewMap = nullptr;
    g_pActionStayOnTop = nullptr;
    g_pBytesPerLineCombo = nullptr;
    g_pElementModeCombo = nullptr;
    g_pQuickOffsetLineEdit = nullptr;
    g_nBytesPerLine = 16;
    g_elementMode = XHexView::ELEMENT_MODE_HEX;
    g_sTextEncoding.clear();

    setWindowTitle(XOptions::getTitle(X_APPLICATIONDISPLAYNAME, X_APPLICATIONVERSION, false));

    setAcceptDrops(true);

    g_xOptions.setName(X_OPTIONSFILE);

    g_xOptions.addID(XOptions::ID_VIEW_STYLE, "Fusion");
    g_xOptions.addID(XOptions::ID_VIEW_QSS, "");
    g_xOptions.addID(XOptions::ID_VIEW_LANG, "System");
    g_xOptions.addID(XOptions::ID_VIEW_FONT_CONTROLS, XOptions::getDefaultFont().toString());
    g_xOptions.addID(XOptions::ID_VIEW_FONT_TABLEVIEWS, XOptions::getMonoFont().toString());
    g_xOptions.addID(XOptions::ID_VIEW_FONT_TREEVIEWS, XOptions::getDefaultFont().toString());
    g_xOptions.addID(XOptions::ID_VIEW_FONT_TEXTEDITS, XOptions::getMonoFont().toString());
    g_xOptions.addID(XOptions::ID_VIEW_STAYONTOP, false);
    g_xOptions.addID(XOptions::ID_VIEW_SHOWLOGO, false);
    g_xOptions.addID(XOptions::ID_FILE_SAVELASTDIRECTORY, true);
    g_xOptions.addID(XOptions::ID_FILE_SAVEBACKUP, true);
    g_xOptions.addID(XOptions::ID_FILE_SAVERECENTFILES, true);

#ifdef Q_OS_WIN
    g_xOptions.addID(XOptions::ID_FILE_CONTEXT, "*");
#endif

    XHexViewOptionsWidget::setDefaultValues(&g_xOptions);

    g_xOptions.load();

    g_xShortcuts.setName(X_SHORTCUTSFILE);
    g_xShortcuts.setNative(g_xOptions.isNative(), g_xOptions.getApplicationDataPath());

    g_xShortcuts.addGroup(XShortcuts::GROUPID_STRINGS);
    //    g_xShortcuts.addGroup(XShortcuts::GROUPID_SIGNATURE);
    g_xShortcuts.addGroup(XShortcuts::GROUPID_HEX);
    //    g_xShortcuts.addGroup(XShortcuts::GROUPID_DISASM);
    //    g_xShortcuts.addGroup(XShortcuts::GROUPID_FIND);

    g_xShortcuts.load();

    g_pInfoMenu = new XInfoMenu(&g_xShortcuts, &g_xOptions);

    // ui->widgetViewer->setGlobal(&g_xShortcuts, &g_xOptions);

    connect(&g_xOptions, SIGNAL(openFile(QString)), this, SLOT(processFile(QString)));
    connect(&g_xOptions, SIGNAL(errorMessage(QString)), this, SLOT(errorMessageSlot(QString)));

    createMenus();

    adjustWindow();

    // ui->widgetViewer->setReadonlyVisible(true);

    if (QCoreApplication::arguments().count() > 1) {
        QString sFileName = QCoreApplication::arguments().at(1);

        processFile(sFileName);
    }
}

GuiMainWindow::~GuiMainWindow()
{
    closeCurrentFile();
    g_xOptions.save();
    g_xShortcuts.save();

    delete g_pInfoMenu;
    delete ui;
}

QAction *GuiMainWindow::addHexAction(QMenu *pMenu, const QString &sText, const char *pMethod, XOptions::ICONTYPE iconType)
{
    QAction *pAction = new QAction(this);
    pAction->setProperty("HEX_METHOD", QByteArray(pMethod));
    pAction->setEnabled(false);
    XOptions::adjustAction(pMenu, pAction, sText, this, SLOT(actionHexCommandSlot()), iconType);
    g_listDocumentActions.append(pAction);

    return pAction;
}

QAction *GuiMainWindow::addSelectionAction(QMenu *pMenu, const QString &sText, const char *pMethod, XOptions::ICONTYPE iconType)
{
    QAction *pAction = addHexAction(pMenu, sText, pMethod, iconType);
    g_listSelectionActions.append(pAction);

    return pAction;
}

QAction *GuiMainWindow::addViewAction(QMenu *pMenu, const QString &sText, const char *pMethod, XOptions::ICONTYPE iconType, qint32 nViewWidget)
{
    QAction *pAction = addHexAction(pMenu, sText, pMethod, iconType);
    pAction->setCheckable(true);
    pAction->setProperty("VIEW_WIDGET", nViewWidget);
    g_listViewActions.append(pAction);

    return pAction;
}

void GuiMainWindow::addMenuShortcutHint(QAction *pAction, quint64 nShortcutId)
{
    if (pAction) {
        pAction->setProperty("MENU_TEXT", pAction->text());
        pAction->setProperty("SHORTCUT_ID", QVariant::fromValue<qulonglong>(nShortcutId));
        g_listShortcutHintActions.append(pAction);
    }
}

void GuiMainWindow::updateMenuShortcutHints()
{
    for (QAction *pAction : g_listShortcutHintActions) {
        const QString sMenuText = pAction->property("MENU_TEXT").toString();
        const quint64 nShortcutId = pAction->property("SHORTCUT_ID").toULongLong();
        const QString sShortcut = g_xShortcuts.getShortcut(nShortcutId).toString(QKeySequence::NativeText);

        pAction->setText(sShortcut.isEmpty() ? sMenuText : QStringLiteral("%1\t%2").arg(sMenuText, sShortcut));
    }
}

void GuiMainWindow::setDocumentActionsEnabled(bool bState)
{
    for (QAction *pAction : g_listDocumentActions) {
        pAction->setEnabled(bState);
    }

    if (bState) {
        selectionChangedSlot();
        viewWidgetsStateChangedSlot();
        syncDisplayActions();
        updateNavigationActions();
    } else {
        for (QAction *pAction : g_listSelectionActions) {
            pAction->setEnabled(false);
        }

        for (QAction *pAction : g_listViewActions) {
            const QSignalBlocker signalBlocker(pAction);
            pAction->setChecked(false);
        }
    }
}

void GuiMainWindow::createMenus()
{
    QMenu *pMenuFile = new QMenu(tr("&File"), ui->menubar);
    QMenu *pMenuEdit = new QMenu(tr("&Edit"), ui->menubar);
    QMenu *pMenuView = new QMenu(tr("&View"), ui->menubar);
    QMenu *pMenuTools = new QMenu(tr("&Tools"), ui->menubar);
    QMenu *pMenuHelp = new QMenu(tr("&Help"), ui->menubar);

    ui->menubar->addAction(pMenuFile->menuAction());
    ui->menubar->addAction(pMenuEdit->menuAction());
    ui->menubar->addAction(pMenuView->menuAction());
    ui->menubar->addAction(pMenuTools->menuAction());
    ui->menubar->addAction(pMenuHelp->menuAction());

    pMenuEdit->menuAction()->setEnabled(false);
    pMenuView->menuAction()->setEnabled(false);
    g_listDocumentActions.append(pMenuEdit->menuAction());
    g_listDocumentActions.append(pMenuView->menuAction());

    QAction *pActionOpen = new QAction(QIcon(QStringLiteral(":/icons/Open.16.16.png")), tr("&Open..."), this);
    g_pActionClose = new QAction(QIcon(QStringLiteral(":/icons/Remove.16.16.png")), tr("Close file"), this);
    QAction *pActionExit = new QAction(QIcon(QStringLiteral(":/icons/Exit.16.16.png")), tr("Exit"), this);
    QAction *pActionOptions = new QAction(QIcon(QStringLiteral(":/icons/Option.16.16.png")), tr("Options..."), this);
    QAction *pActionAbout = new QAction(QIcon(QStringLiteral(":/icons/Info.16.16.png")), tr("About XHexViewer"), this);
    QAction *pActionAboutQt = new QAction(tr("About Qt"), this);
    QAction *pActionShortcuts = new QAction(QIcon(QStringLiteral(":/icons/Shortcut.16.16.png")), tr("Keyboard shortcuts..."), this);
    QAction *pActionFileInformation = new QAction(QIcon(QStringLiteral(":/icons/Info.16.16.png")), tr("File &information..."), this);
    g_pActionNavigateBack = new QAction(QIcon(QStringLiteral(":/icons/Backward.16.16.png")), tr("&Back"), this);
    g_pActionNavigateForward = new QAction(QIcon(QStringLiteral(":/icons/Forward.16.16.png")), tr("&Forward"), this);

    pActionOpen->setShortcut(QKeySequence::Open);
    g_pActionClose->setShortcut(QKeySequence::Close);
    pActionExit->setShortcut(QKeySequence::Quit);
    pActionOptions->setShortcut(QKeySequence(QStringLiteral("Ctrl+,")));
    g_pActionNavigateBack->setShortcut(QKeySequence::Back);
    g_pActionNavigateForward->setShortcut(QKeySequence::Forward);
    g_pActionClose->setEnabled(false);
    g_pActionNavigateBack->setEnabled(false);
    g_pActionNavigateForward->setEnabled(false);

    pActionOpen->setStatusTip(tr("Open a binary file"));
    g_pActionClose->setStatusTip(tr("Close the current file"));
    pActionOptions->setStatusTip(tr("Configure XHexViewer"));
    pActionFileInformation->setStatusTip(tr("Show information about the current file"));
    g_pActionNavigateBack->setStatusTip(tr("Return to the previous visited location"));
    g_pActionNavigateForward->setStatusTip(tr("Move to the next visited location"));
    pActionFileInformation->setEnabled(false);
    g_listDocumentActions.append(pActionFileInformation);
    g_listDocumentActions.append(g_pActionNavigateBack);
    g_listDocumentActions.append(g_pActionNavigateForward);

    QMenu *pRecentFilesMenu = g_xOptions.createRecentFilesMenu(this);
    pMenuFile->addAction(pActionOpen);
    pMenuFile->addMenu(pRecentFilesMenu);
    pMenuFile->addMenu(g_pInfoMenu->createMenu(this));
    pMenuFile->addSeparator();
    pMenuFile->addAction(pActionFileInformation);
    QMenu *pMenuChecksums = pMenuFile->addMenu(QIcon(QStringLiteral(":/icons/Hash.16.16.png")), tr("Copy file &checksum"));

    auto addChecksumAction = [this, pMenuChecksums](const QString &sText, const QString &sName, QCryptographicHash::Algorithm algorithm) {
        QAction *pAction = new QAction(tr("Copy %1").arg(sText), this);
        pAction->setData((qint32)algorithm);
        pAction->setProperty("HASH_NAME", sName);
        pAction->setStatusTip(tr("Calculate and copy the %1 checksum of the current file").arg(sText));
        pMenuChecksums->addAction(pAction);
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionCopyChecksumSlot()));
    };

    addChecksumAction(QStringLiteral("MD5"), QStringLiteral("MD5"), QCryptographicHash::Md5);
    addChecksumAction(QStringLiteral("SHA-1"), QStringLiteral("SHA-1"), QCryptographicHash::Sha1);
    addChecksumAction(QStringLiteral("SHA-256"), QStringLiteral("SHA-256"), QCryptographicHash::Sha256);
    pMenuChecksums->menuAction()->setEnabled(false);
    g_listDocumentActions.append(pMenuChecksums->menuAction());
    QMenu *pMenuFileLocation = pMenuFile->addMenu(QIcon(QStringLiteral(":/icons/Path.16.16.png")), tr("File &location"));
    QAction *pActionShowInFolder = new QAction(QIcon(QStringLiteral(":/icons/Open.16.16.png")), tr("Show in &folder"), this);
    QAction *pActionCopyFilePath = new QAction(QIcon(QStringLiteral(":/icons/Copy.16.16.png")), tr("Copy full &path"), this);
    QAction *pActionCopyFileName = new QAction(QIcon(QStringLiteral(":/icons/Copy.16.16.png")), tr("Copy file &name"), this);
    QAction *pActionCopyFolderPath = new QAction(QIcon(QStringLiteral(":/icons/Copy.16.16.png")), tr("Copy containing fo&lder"), this);
    pActionShowInFolder->setStatusTip(tr("Show the current file in its containing folder"));
    pActionCopyFilePath->setStatusTip(tr("Copy the full path of the current file"));
    pActionCopyFileName->setStatusTip(tr("Copy the current file name"));
    pActionCopyFolderPath->setStatusTip(tr("Copy the path of the containing folder"));
    pMenuFileLocation->addAction(pActionShowInFolder);
    pMenuFileLocation->addSeparator();
    pMenuFileLocation->addAction(pActionCopyFilePath);
    pMenuFileLocation->addAction(pActionCopyFileName);
    pMenuFileLocation->addAction(pActionCopyFolderPath);
    pMenuFileLocation->menuAction()->setEnabled(false);
    g_listDocumentActions.append(pMenuFileLocation->menuAction());
    pMenuFile->addSeparator();
    QAction *pActionReload = addHexAction(pMenuFile, tr("Reload view"), "reloadView", XOptions::ICONTYPE_RELOAD);
    pActionReload->setShortcut(QKeySequence(Qt::Key_F5));
    pActionReload->setToolTip(tr("Reload view"));
    pActionReload->setStatusTip(tr("Reload the current hexadecimal view"));
    QAction *pActionExportSelection = addSelectionAction(pMenuFile, tr("Export selection..."), "_dumpToFileSlot", XOptions::ICONTYPE_DUMPTOFILE);
    pActionExportSelection->setStatusTip(tr("Save the selected bytes to a file"));
    pMenuFile->addAction(g_pActionClose);
    pMenuFile->addSeparator();
    pMenuFile->addAction(pActionExit);

    QMenu *pMenuFind = pMenuEdit->addMenu(QIcon(QStringLiteral(":/icons/Search.16.16.png")), tr("&Find"));
    QAction *pActionFindText = addHexAction(pMenuFind, tr("Text..."), "_findStringSlot", XOptions::ICONTYPE_STRING);
    addMenuShortcutHint(pActionFindText, X_ID_HEX_FIND_STRING);
    pActionFindText->setToolTip(tr("Find text"));
    pActionFindText->setStatusTip(tr("Search for text in the current file"));
    addHexAction(pMenuFind, tr("Hex signature..."), "_findSignatureSlot", XOptions::ICONTYPE_SIGNATURE);
    addHexAction(pMenuFind, tr("Value..."), "_findValueSlot", XOptions::ICONTYPE_VALUE);
    pMenuFind->addSeparator();
    QAction *pActionFindNext = addHexAction(pMenuFind, tr("Find next"), "_findNextSlot", XOptions::ICONTYPE_NEXT);
    addMenuShortcutHint(pActionFindNext, X_ID_HEX_FIND_NEXT);

    QMenu *pMenuGoTo = pMenuEdit->addMenu(QIcon(QStringLiteral(":/icons/Goto.16.16.png")), tr("&Go to"));
    pMenuGoTo->addAction(g_pActionNavigateBack);
    pMenuGoTo->addAction(g_pActionNavigateForward);
    pMenuGoTo->addSeparator();
    QAction *pActionGoToOffset = addHexAction(pMenuGoTo, tr("Offset..."), "_goToOffsetSlot", XOptions::ICONTYPE_OFFSET);
    addMenuShortcutHint(pActionGoToOffset, X_ID_HEX_GOTO_OFFSET);
    pActionGoToOffset->setToolTip(tr("Go to offset"));
    pActionGoToOffset->setStatusTip(tr("Jump to a file offset"));
    addHexAction(pMenuGoTo, tr("Address..."), "_goToAddressSlot", XOptions::ICONTYPE_ADDRESS);
    pMenuGoTo->addSeparator();
    QAction *pActionGoToFileStart = new QAction(QIcon(QStringLiteral(":/icons/Backward.16.16.png")), tr("File &start"), this);
    QAction *pActionGoToFileEnd = new QAction(QIcon(QStringLiteral(":/icons/Forward.16.16.png")), tr("File &end"), this);
    pActionGoToFileStart->setShortcut(QKeySequence::MoveToStartOfDocument);
    pActionGoToFileEnd->setShortcut(QKeySequence::MoveToEndOfDocument);
    pActionGoToFileStart->setStatusTip(tr("Jump to the first byte of the file"));
    pActionGoToFileEnd->setStatusTip(tr("Jump to the last byte of the file"));
    pActionGoToFileStart->setEnabled(false);
    pActionGoToFileEnd->setEnabled(false);
    pMenuGoTo->addAction(pActionGoToFileStart);
    pMenuGoTo->addAction(pActionGoToFileEnd);
    g_listDocumentActions.append(pActionGoToFileStart);
    g_listDocumentActions.append(pActionGoToFileEnd);
    pMenuGoTo->addSeparator();
    addSelectionAction(pMenuGoTo, tr("Selection start"), "_goToSelectionStart", XOptions::ICONTYPE_BACKWARD);
    addSelectionAction(pMenuGoTo, tr("Selection end"), "_goToSelectionEnd", XOptions::ICONTYPE_FORWARD);

    pMenuEdit->addSeparator();
    QAction *pActionSelectAll = addHexAction(pMenuEdit, tr("Select &all"), "_selectAllSlot", XOptions::ICONTYPE_SELECT);
    addMenuShortcutHint(pActionSelectAll, X_ID_HEX_SELECT_ALL);

    QMenu *pMenuCopy = pMenuEdit->addMenu(QIcon(QStringLiteral(":/icons/Copy.16.16.png")), tr("&Copy"));
    addSelectionAction(pMenuCopy, tr("Selected data..."), "_copyDataSlot", XOptions::ICONTYPE_DATA);
    QMenu *pMenuCopySelectionAs = pMenuCopy->addMenu(QIcon(QStringLiteral(":/icons/Copy.16.16.png")), tr("Copy selection &as"));
    pMenuCopySelectionAs->menuAction()->setEnabled(false);
    g_listDocumentActions.append(pMenuCopySelectionAs->menuAction());
    g_listSelectionActions.append(pMenuCopySelectionAs->menuAction());

    auto addCopySelectionFormatAction = [this, pMenuCopySelectionAs](const QString &sText, qint32 nFormat, const QString &sStatusTip) {
        QAction *pAction = new QAction(sText, this);
        pAction->setData(nFormat);
        pAction->setEnabled(false);
        pAction->setStatusTip(sStatusTip);
        pMenuCopySelectionAs->addAction(pAction);
        g_listDocumentActions.append(pAction);
        g_listSelectionActions.append(pAction);
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionCopySelectionFormattedSlot()));
    };

    addCopySelectionFormatAction(tr("Hex bytes (spaced)"), 0, tr("Copy the selected bytes as uppercase hexadecimal separated by spaces"));
    addCopySelectionFormatAction(tr("Hex string (compact)"), 1, tr("Copy the selected bytes as one continuous uppercase hexadecimal string"));
    pMenuCopySelectionAs->addSeparator();
    addCopySelectionFormatAction(tr("C/C++ byte array"), 2, tr("Copy the selected bytes as a C/C++ initializer list"));
    addCopySelectionFormatAction(tr("Escaped bytes (\\xNN)"), 3, tr("Copy the selected bytes using hexadecimal escape sequences"));
    pMenuCopy->addSeparator();
    QAction *pActionCopySelectionRange = new QAction(QIcon(QStringLiteral(":/icons/Offset.16.16.png")), tr("Selection range"), this);
    pActionCopySelectionRange->setEnabled(false);
    pActionCopySelectionRange->setStatusTip(tr("Copy the inclusive start and end offsets of the selection"));
    QAction *pActionCopySelectionSize = new QAction(QIcon(QStringLiteral(":/icons/Size.16.16.png")), tr("Selection size"), this);
    pActionCopySelectionSize->setEnabled(false);
    pActionCopySelectionSize->setStatusTip(tr("Copy the size of the current selection"));
    pMenuCopy->addAction(pActionCopySelectionRange);
    pMenuCopy->addAction(pActionCopySelectionSize);
    g_listDocumentActions.append(pActionCopySelectionRange);
    g_listDocumentActions.append(pActionCopySelectionSize);
    g_listSelectionActions.append(pActionCopySelectionRange);
    g_listSelectionActions.append(pActionCopySelectionSize);
    pMenuCopy->addSeparator();
    addHexAction(pMenuCopy, tr("Offset"), "_copyOffsetSlot", XOptions::ICONTYPE_OFFSET);
    addHexAction(pMenuCopy, tr("Address"), "_copyAddressSlot", XOptions::ICONTYPE_ADDRESS);
    addHexAction(pMenuCopy, tr("Relative address"), "_copyRelAddressSlot", XOptions::ICONTYPE_ADDRESS);
    pMenuEdit->addSeparator();
    QAction *pActionHexSignature = addSelectionAction(pMenuEdit, tr("Create &hex signature..."), "_hexSignatureSlot", XOptions::ICONTYPE_SIGNATURE);
    pActionHexSignature->setStatusTip(tr("Create a hexadecimal signature from the selection"));

    QMenu *pMenuDisplay = pMenuView->addMenu(QIcon(QStringLiteral(":/icons/Table.16.16.png")), tr("&Display"));
    QMenu *pMenuElementMode = pMenuDisplay->addMenu(QIcon(QStringLiteral(":/icons/Data.16.16.png")), tr("&Element format"));
    QActionGroup *pElementModeGroup = new QActionGroup(this);
    pElementModeGroup->setExclusive(true);

    auto addElementModeAction = [this, pMenuElementMode, pElementModeGroup](const QString &sText, const QString &sComboText, XHexView::ELEMENT_MODE mode) {
        QAction *pAction = new QAction(sText, this);
        pAction->setCheckable(true);
        pAction->setData((qint32)mode);
        pAction->setProperty("COMBO_TEXT", sComboText);
        pAction->setStatusTip(tr("Interpret table elements as %1").arg(sText.toLower()));
        pElementModeGroup->addAction(pAction);
        pMenuElementMode->addAction(pAction);
        g_listElementModeActions.append(pAction);
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionElementModeSlot()));
    };

    addElementModeAction(tr("Hex bytes"), tr("Hex"), XHexView::ELEMENT_MODE_HEX);
    pMenuElementMode->addSeparator();
    addElementModeAction(tr("Hex word (16-bit)"), tr("Hex16"), XHexView::ELEMENT_MODE_WORD);
    addElementModeAction(tr("Hex dword (32-bit)"), tr("Hex32"), XHexView::ELEMENT_MODE_DWORD);
    addElementModeAction(tr("Hex qword (64-bit)"), tr("Hex64"), XHexView::ELEMENT_MODE_QWORD);
    pMenuElementMode->addSeparator();
    addElementModeAction(tr("Unsigned 8-bit"), tr("u8"), XHexView::ELEMENT_MODE_UINT8);
    addElementModeAction(tr("Signed 8-bit"), tr("i8"), XHexView::ELEMENT_MODE_INT8);
    addElementModeAction(tr("Unsigned 16-bit"), tr("u16"), XHexView::ELEMENT_MODE_UINT16);
    addElementModeAction(tr("Signed 16-bit"), tr("i16"), XHexView::ELEMENT_MODE_INT16);
    addElementModeAction(tr("Unsigned 32-bit"), tr("u32"), XHexView::ELEMENT_MODE_UINT32);
    addElementModeAction(tr("Signed 32-bit"), tr("i32"), XHexView::ELEMENT_MODE_INT32);
    addElementModeAction(tr("Unsigned 64-bit"), tr("u64"), XHexView::ELEMENT_MODE_UINT64);
    addElementModeAction(tr("Signed 64-bit"), tr("i64"), XHexView::ELEMENT_MODE_INT64);

    QMenu *pMenuBytesPerLine = pMenuDisplay->addMenu(tr("&Bytes per line"));
    QActionGroup *pBytesPerLineGroup = new QActionGroup(this);
    pBytesPerLineGroup->setExclusive(true);

    const QList<qint32> listBytesPerLine = {8, 16, 24, 32, 48, 64};
    for (qint32 nBytesPerLine : listBytesPerLine) {
        QAction *pActionBytesPerLine = new QAction(tr("%1 bytes").arg(nBytesPerLine), this);
        pActionBytesPerLine->setCheckable(true);
        pActionBytesPerLine->setData(nBytesPerLine);
        pActionBytesPerLine->setStatusTip(tr("Show %1 bytes on each row").arg(nBytesPerLine));
        pBytesPerLineGroup->addAction(pActionBytesPerLine);
        pMenuBytesPerLine->addAction(pActionBytesPerLine);
        g_listBytesPerLineActions.append(pActionBytesPerLine);
        connect(pActionBytesPerLine, SIGNAL(triggered()), this, SLOT(actionBytesPerLineSlot()));
    }

    QMenu *pMenuTextEncoding = pMenuDisplay->addMenu(QIcon(QStringLiteral(":/icons/String.16.16.png")), tr("&Text encoding"));
    QActionGroup *pTextEncodingGroup = new QActionGroup(this);
    pTextEncodingGroup->setExclusive(true);

    auto addTextEncodingAction = [this, pMenuTextEncoding, pTextEncodingGroup](const QString &sText, const QString &sCodePage) {
        QAction *pAction = new QAction(sText, this);
        pAction->setCheckable(true);
        pAction->setData(sCodePage);
        pAction->setStatusTip(tr("Decode the symbols column using %1").arg(sText));
        pTextEncodingGroup->addAction(pAction);
        pMenuTextEncoding->addAction(pAction);
        g_listTextEncodingActions.append(pAction);
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionTextEncodingSlot()));
    };

    addTextEncodingAction(tr("System / default"), QString());
    pMenuTextEncoding->addSeparator();
    addTextEncodingAction(QStringLiteral("UTF-8"), QStringLiteral("UTF-8"));
    addTextEncodingAction(QStringLiteral("UTF-16 LE"), QStringLiteral("UTF-16LE"));
    addTextEncodingAction(QStringLiteral("UTF-16 BE"), QStringLiteral("UTF-16BE"));
    pMenuTextEncoding->addSeparator();
    addTextEncodingAction(QStringLiteral("ISO-8859-1"), QStringLiteral("ISO-8859-1"));
    addTextEncodingAction(QStringLiteral("Windows-1252"), QStringLiteral("windows-1252"));

    pMenuDisplay->addSeparator();
    QMenu *pMenuLocationMode = pMenuDisplay->addMenu(QIcon(QStringLiteral(":/icons/Offset.16.16.png")), tr("&Location labels"));
    QActionGroup *pLocationModeGroup = new QActionGroup(this);
    pLocationModeGroup->setExclusive(true);

    auto addLocationModeAction = [this, pMenuLocationMode, pLocationModeGroup](const QString &sText, XBinaryView::LOCMODE mode) {
        QAction *pAction = new QAction(sText, this);
        pAction->setCheckable(true);
        pAction->setData((qint32)mode);
        pAction->setStatusTip(tr("Show row locations as %1").arg(sText.toLower()));
        pLocationModeGroup->addAction(pAction);
        pMenuLocationMode->addAction(pAction);
        g_listLocationModeActions.append(pAction);
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionLocationModeSlot()));
    };

    addLocationModeAction(tr("File offsets"), XBinaryView::LOCMODE_OFFSET);
    addLocationModeAction(tr("Virtual addresses"), XBinaryView::LOCMODE_ADDRESS);

    QMenu *pMenuLocationBase = pMenuDisplay->addMenu(QIcon(QStringLiteral(":/icons/Value.16.16.png")), tr("Location &base"));
    QActionGroup *pLocationBaseGroup = new QActionGroup(this);
    pLocationBaseGroup->setExclusive(true);

    auto addLocationBaseAction = [this, pMenuLocationBase, pLocationBaseGroup](const QString &sText, qint32 nBase) {
        QAction *pAction = new QAction(sText, this);
        pAction->setCheckable(true);
        pAction->setData(nBase);
        pAction->setStatusTip(tr("Display locations in base %1").arg(nBase));
        pLocationBaseGroup->addAction(pAction);
        pMenuLocationBase->addAction(pAction);
        g_listLocationBaseActions.append(pAction);
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionLocationBaseSlot()));
    };

    addLocationBaseAction(tr("Hexadecimal (base 16)"), 16);
    addLocationBaseAction(tr("Decimal (base 10)"), 10);

    pMenuDisplay->addSeparator();
    g_pActionOverviewMap = new QAction(QIcon(QStringLiteral(":/icons/Entropy.16.16.png")), tr("&Overview map"), this);
    g_pActionOverviewMap->setCheckable(true);
    g_pActionOverviewMap->setChecked(true);
    g_pActionOverviewMap->setStatusTip(tr("Show or hide the file overview map"));
    pMenuDisplay->addAction(g_pActionOverviewMap);
    pMenuDisplay->addSeparator();
    QAction *pActionResetDisplay = new QAction(QIcon(QStringLiteral(":/icons/Refresh.16.16.png")), tr("&Reset display"), this);
    pActionResetDisplay->setStatusTip(tr("Restore hex bytes, default text encoding, file offsets in hexadecimal, 16 bytes per line, and the overview map"));
    pMenuDisplay->addAction(pActionResetDisplay);
    pMenuView->addSeparator();

    QMenu *pMenuZoom = pMenuView->addMenu(QIcon(QStringLiteral(":/icons/Resize.16.16.png")), tr("&Zoom"));
    QAction *pActionZoomIn = new QAction(QIcon(QStringLiteral(":/icons/Add.16.16.png")), tr("Zoom &in"), this);
    pActionZoomIn->setShortcut(QKeySequence::ZoomIn);
    pActionZoomIn->setStatusTip(tr("Increase the hexadecimal view font size"));
    QAction *pActionZoomOut = new QAction(QIcon(QStringLiteral(":/icons/Remove.16.16.png")), tr("Zoom &out"), this);
    pActionZoomOut->setShortcut(QKeySequence::ZoomOut);
    pActionZoomOut->setStatusTip(tr("Decrease the hexadecimal view font size"));
    QAction *pActionResetZoom = new QAction(QIcon(QStringLiteral(":/icons/Refresh.16.16.png")), tr("&Reset zoom"), this);
    pActionResetZoom->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_0));
    pActionResetZoom->setStatusTip(tr("Restore the configured hexadecimal view font size"));
    pMenuZoom->addAction(pActionZoomIn);
    pMenuZoom->addAction(pActionZoomOut);
    pMenuZoom->addSeparator();
    pMenuZoom->addAction(pActionResetZoom);
    pMenuView->addSeparator();

    QAction *pActionDataInspector =
        addViewAction(pMenuView, tr("Data &inspector"), "_dataInspector", XOptions::ICONTYPE_INSPECTOR, XDeviceTableEditView::VIEWWIDGET_DATAINSPECTOR);
    pActionDataInspector->setToolTip(tr("Data inspector"));
    pActionDataInspector->setStatusTip(tr("Inspect the selected bytes as common data types"));
    QAction *pActionStrings = addViewAction(pMenuView, tr("&Strings"), "_strings", XOptions::ICONTYPE_STRING, XDeviceTableEditView::VIEWWIDGET_STRINGS);
    pActionStrings->setToolTip(tr("Strings"));
    pActionStrings->setStatusTip(tr("Extract strings from the current file"));
    addViewAction(pMenuView, tr("Data &converter"), "_dataConvertor", XOptions::ICONTYPE_CONVERTOR, XDeviceTableEditView::VIEWWIDGET_DATACONVERTOR);
    addViewAction(pMenuView, tr("&Visualization"), "_visualization", XOptions::ICONTYPE_VISUALIZATION, XDeviceTableEditView::VIEWWIDGET_VISUALIZATION);
    pMenuView->addSeparator();
    addViewAction(pMenuView, tr("&Multi-search"), "_multisearch", XOptions::ICONTYPE_SEARCH, XDeviceTableEditView::VIEWWIDGET_MULTISEARCH);

    QMenu *pMenuBookmarks = pMenuView->addMenu(QIcon(QStringLiteral(":/icons/Bookmark.16.16.png")), tr("Bookmarks"));
    addHexAction(pMenuBookmarks, tr("Add bookmark..."), "_bookmarkNew", XOptions::ICONTYPE_ADD);
    addViewAction(pMenuBookmarks, tr("Manage bookmarks..."), "_bookmarkList", XOptions::ICONTYPE_LIST, XDeviceTableEditView::VIEWWIDGET_BOOKMARKS);

    addHexAction(pMenuTools, tr("Structures..."), "_structs", XOptions::ICONTYPE_STRUCTS);
    pMenuTools->addSeparator();
    pMenuTools->addAction(pActionShortcuts);
    pMenuTools->addAction(pActionOptions);
    pMenuHelp->addAction(pActionAbout);
    pMenuHelp->addSeparator();
    pMenuHelp->addAction(pActionAboutQt);

    QToolBar *pToolBar = addToolBar(tr("Main toolbar"));
    pToolBar->setObjectName(QStringLiteral("mainToolBar"));
    pToolBar->setMovable(false);
    pToolBar->setFloatable(false);
    pToolBar->setIconSize(QSize(18, 18));
    pToolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);

    auto createToolBarHexAction = [this](QAction *pSourceAction, const QString &sText, const QString &sToolTip, const QString &sStatusTip) {
        QAction *pAction = new QAction(pSourceAction->icon(), sText, this);
        pAction->setProperty("HEX_METHOD", pSourceAction->property("HEX_METHOD"));
        pAction->setEnabled(false);
        pAction->setToolTip(sToolTip);
        pAction->setStatusTip(sStatusTip);
        const QVariant viewWidget = pSourceAction->property("VIEW_WIDGET");
        if (viewWidget.isValid()) {
            pAction->setCheckable(true);
            pAction->setProperty("VIEW_WIDGET", viewWidget);
            g_listViewActions.append(pAction);
        }
        connect(pAction, SIGNAL(triggered()), this, SLOT(actionHexCommandSlot()));
        g_listDocumentActions.append(pAction);

        return pAction;
    };

    QAction *pToolActionReload = createToolBarHexAction(pActionReload, tr("Reload"), tr("Reload view"), tr("Reload the current hexadecimal view"));
    QAction *pToolActionFind = createToolBarHexAction(pActionFindText, tr("Find"), tr("Find text"), tr("Search for text in the current file"));
    QAction *pToolActionGoTo = createToolBarHexAction(pActionGoToOffset, tr("Go to"), tr("Go to offset"), tr("Jump to a file offset"));
    QAction *pToolActionInspector =
        createToolBarHexAction(pActionDataInspector, tr("Inspector"), tr("Data inspector"), tr("Inspect the selected bytes as common data types"));
    QAction *pToolActionStrings = createToolBarHexAction(pActionStrings, tr("Strings"), tr("Strings"), tr("Extract strings from the current file"));

    pToolBar->addAction(pActionOpen);
    pToolBar->addAction(g_pActionClose);
    pToolBar->addSeparator();
    pToolBar->addAction(pToolActionReload);
    pToolBar->addSeparator();
    pToolBar->addAction(g_pActionNavigateBack);
    pToolBar->addAction(g_pActionNavigateForward);
    pToolBar->addSeparator();
    pToolBar->addAction(pToolActionFind);
    pToolBar->addAction(pToolActionGoTo);
    pToolBar->addSeparator();
    pToolBar->addAction(pToolActionInspector);
    pToolBar->addAction(pToolActionStrings);

    pToolBar->addSeparator();
    g_pQuickOffsetLineEdit = new QLineEdit(pToolBar);
    g_pQuickOffsetLineEdit->setObjectName(QStringLiteral("quickOffsetLineEdit"));
    g_pQuickOffsetLineEdit->setAccessibleName(tr("Quick offset"));
    g_pQuickOffsetLineEdit->setEnabled(false);
    g_pQuickOffsetLineEdit->setMinimumWidth(76);
    g_pQuickOffsetLineEdit->setMaximumWidth(88);
    g_pQuickOffsetLineEdit->setPlaceholderText(QStringLiteral("0x0"));
    g_pQuickOffsetLineEdit->setClearButtonEnabled(true);
    g_pQuickOffsetLineEdit->setToolTip(tr("Quick offset: enter hexadecimal, or append 'd' for decimal, then press Enter"));
    QAction *pActionQuickGo = new QAction(QIcon(QStringLiteral(":/icons/Goto.16.16.png")), tr("Go"), this);
    pActionQuickGo->setEnabled(false);
    pActionQuickGo->setToolTip(tr("Go to offset"));
    pActionQuickGo->setStatusTip(tr("Jump to the offset entered in the toolbar"));
    pToolBar->addWidget(g_pQuickOffsetLineEdit);
    pToolBar->addAction(pActionQuickGo);
    QToolButton *pQuickGoButton = qobject_cast<QToolButton *>(pToolBar->widgetForAction(pActionQuickGo));
    if (pQuickGoButton) {
        pQuickGoButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    }
    g_listDocumentActions.append(pActionQuickGo);

    QWidget *pToolBarSpacer = new QWidget(pToolBar);
    pToolBarSpacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    pToolBar->addWidget(pToolBarSpacer);
    pToolBar->addAction(pActionOptions);

    pMenuView->addSeparator();
    QMenu *pMenuInterface = pMenuView->addMenu(QIcon(QStringLiteral(":/icons/Tool.16.16.png")), tr("Inter&face"));
    QAction *pActionToolBar = pToolBar->toggleViewAction();
    pActionToolBar->setText(tr("Main &toolbar"));
    pActionToolBar->setStatusTip(tr("Show or hide the main toolbar"));
    QAction *pActionStatusBar = new QAction(tr("&Status bar"), this);
    pActionStatusBar->setCheckable(true);
    pActionStatusBar->setChecked(statusBar()->isVisible());
    pActionStatusBar->setStatusTip(tr("Show or hide the status bar"));
    g_pActionStayOnTop = new QAction(tr("Always on &top"), this);
    g_pActionStayOnTop->setCheckable(true);
    g_pActionStayOnTop->setChecked(g_xOptions.isStayOnTop());
    g_pActionStayOnTop->setStatusTip(tr("Keep the main window above other windows"));
    QAction *pActionFullScreen = new QAction(tr("&Full screen"), this);
    pActionFullScreen->setCheckable(true);
    pActionFullScreen->setShortcut(QKeySequence(Qt::Key_F11));
    pActionFullScreen->setStatusTip(tr("Toggle full-screen mode"));
    pMenuInterface->addAction(pActionToolBar);
    pMenuInterface->addAction(pActionStatusBar);
    pMenuInterface->addAction(g_pActionStayOnTop);
    pMenuInterface->addSeparator();
    pMenuInterface->addAction(pActionFullScreen);

    ui->toolButtonRecentWelcome->setMenu(pRecentFilesMenu);
    ui->toolButtonRecentWelcome->setPopupMode(QToolButton::InstantPopup);
    ui->toolButtonRecentWelcome->setToolTip(tr("Open a recent file"));
    ui->toolButtonRecentWelcome->setEnabled(g_xOptions.getRecentFiles().count() > 0);

    connect(pActionOpen, SIGNAL(triggered()), this, SLOT(actionOpenSlot()));
    connect(g_pActionClose, SIGNAL(triggered()), this, SLOT(actionCloseSlot()));
    connect(pActionExit, SIGNAL(triggered()), this, SLOT(actionExitSlot()));
    connect(pActionOptions, SIGNAL(triggered()), this, SLOT(actionOptionsSlot()));
    connect(pActionAbout, SIGNAL(triggered()), this, SLOT(actionAboutSlot()));
    connect(pActionFileInformation, SIGNAL(triggered()), this, SLOT(actionFileInformationSlot()));
    connect(pActionShowInFolder, SIGNAL(triggered()), this, SLOT(actionShowInFolderSlot()));
    connect(pActionCopyFilePath, SIGNAL(triggered()), this, SLOT(actionCopyFilePathSlot()));
    connect(pActionCopyFileName, SIGNAL(triggered()), this, SLOT(actionCopyFileNameSlot()));
    connect(pActionCopyFolderPath, SIGNAL(triggered()), this, SLOT(actionCopyFolderPathSlot()));
    connect(g_pActionNavigateBack, SIGNAL(triggered()), this, SLOT(actionNavigateBackSlot()));
    connect(g_pActionNavigateForward, SIGNAL(triggered()), this, SLOT(actionNavigateForwardSlot()));
    connect(pActionGoToFileStart, SIGNAL(triggered()), this, SLOT(actionGoToFileStartSlot()));
    connect(pActionGoToFileEnd, SIGNAL(triggered()), this, SLOT(actionGoToFileEndSlot()));
    connect(pActionQuickGo, SIGNAL(triggered()), this, SLOT(actionQuickGoSlot()));
    connect(g_pQuickOffsetLineEdit, SIGNAL(returnPressed()), this, SLOT(actionQuickGoSlot()));
    connect(pActionCopySelectionRange, SIGNAL(triggered()), this, SLOT(actionCopySelectionRangeSlot()));
    connect(pActionCopySelectionSize, SIGNAL(triggered()), this, SLOT(actionCopySelectionSizeSlot()));
    connect(g_pActionOverviewMap, SIGNAL(toggled(bool)), this, SLOT(actionOverviewMapSlot(bool)));
    connect(pActionResetDisplay, SIGNAL(triggered()), this, SLOT(actionResetDisplaySlot()));
    connect(pActionZoomIn, SIGNAL(triggered()), this, SLOT(actionZoomInSlot()));
    connect(pActionZoomOut, SIGNAL(triggered()), this, SLOT(actionZoomOutSlot()));
    connect(pActionResetZoom, SIGNAL(triggered()), this, SLOT(actionResetZoomSlot()));
    connect(g_pActionStayOnTop, SIGNAL(toggled(bool)), this, SLOT(actionStayOnTopSlot(bool)));
    connect(pActionShortcuts, SIGNAL(triggered()), this, SLOT(actionShortcutsSlot()));
    connect(pActionAboutQt, &QAction::triggered, qApp, &QApplication::aboutQt);
    connect(ui->pushButtonOpenWelcome, SIGNAL(clicked()), this, SLOT(actionOpenSlot()));
    connect(pRecentFilesMenu, SIGNAL(triggered(QAction *)), this, SLOT(updateRecentFilesState()), Qt::QueuedConnection);
    connect(pActionStatusBar, &QAction::toggled, statusBar(), &QStatusBar::setVisible);
    connect(pActionFullScreen, &QAction::toggled, this,
            [this](bool bChecked) { setWindowState(bChecked ? (windowState() | Qt::WindowFullScreen) : (windowState() & ~Qt::WindowFullScreen)); });
    connect(pMenuInterface, &QMenu::aboutToShow, this, [this, pActionStatusBar, pActionFullScreen]() {
        const QSignalBlocker statusBarBlocker(pActionStatusBar);
        const QSignalBlocker stayOnTopBlocker(g_pActionStayOnTop);
        const QSignalBlocker fullScreenBlocker(pActionFullScreen);
        pActionStatusBar->setChecked(statusBar()->isVisible());
        g_pActionStayOnTop->setChecked(g_xOptions.isStayOnTop());
        pActionFullScreen->setChecked(isFullScreen());
    });

    updateMenuShortcutHints();
}

void GuiMainWindow::actionOpenSlot()
{
    QString sDirectory = g_xOptions.getLastDirectory();

    QString sFileName = QFileDialog::getOpenFileName(this, tr("Open file") + QString("..."), sDirectory, tr("All files") + QString(" (*)"));

    if (!sFileName.isEmpty()) {
        processFile(sFileName);
    }
}

void GuiMainWindow::actionCloseSlot()
{
    closeCurrentFile();
}

void GuiMainWindow::actionExitSlot()
{
    this->close();
}

void GuiMainWindow::actionOptionsSlot()
{
    DialogOptions dialogOptions(this, &g_xOptions);
    dialogOptions.setGlobal(&g_xShortcuts, &g_xOptions);
    dialogOptions.exec();

    // ui->widgetViewer->adjustView();
    adjustWindow();
}

void GuiMainWindow::actionAboutSlot()
{
    DialogAbout dialogAbout(this);
    dialogAbout.exec();
}

void GuiMainWindow::actionFileInformationSlot()
{
    if (!g_pFile) {
        return;
    }

    const QFileInfo fileInfo(g_pFile->fileName());
    const QLocale locale;
    const QString sModified = fileInfo.lastModified().isValid() ? locale.toString(fileInfo.lastModified(), QLocale::ShortFormat) : tr("Unknown");
    const QString sInformation = tr("Name: %1\nFolder: %2\nFull path: %3\nSize: %4 bytes\nLast modified: %5\nViewer mode: Read-only")
                                     .arg(fileInfo.fileName(), QDir::toNativeSeparators(fileInfo.absolutePath()), QDir::toNativeSeparators(fileInfo.absoluteFilePath()),
                                          locale.toString(fileInfo.size()), sModified);

    QMessageBox::information(this, tr("File information"), sInformation);
}

void GuiMainWindow::copyTextToClipboard(const QString &sText, const QString &sMessage)
{
    QApplication::clipboard()->setText(sText);
    statusBar()->showMessage(sMessage, 3000);
}

bool GuiMainWindow::readSelectedBytes(QByteArray *pData, qint64 nMaximumSize)
{
    if (!pData || !g_pFile || !g_pHexView) {
        return false;
    }

    const XDeviceTableView::DEVICESTATE state = g_pHexView->getDeviceState();
    if (state.nSelectionSize <= 0) {
        return false;
    }

    if (state.nSelectionSize > nMaximumSize) {
        statusBar()->showMessage(tr("Selection is too large for formatted clipboard copy (maximum 1 MiB)"), 4000);
        return false;
    }

    QFile selectionFile(g_pFile->fileName());
    if (!selectionFile.open(QIODevice::ReadOnly) || !selectionFile.seek((qint64)state.nSelectionDeviceOffset)) {
        statusBar()->showMessage(tr("Cannot read the selected bytes"), 3000);
        return false;
    }

    *pData = selectionFile.read(state.nSelectionSize);
    if (pData->size() != state.nSelectionSize) {
        pData->clear();
        statusBar()->showMessage(tr("Cannot read the complete selection"), 3000);
        return false;
    }

    return true;
}

void GuiMainWindow::actionCopyFilePathSlot()
{
    if (g_pFile) {
        const QString sFileName = QDir::toNativeSeparators(QFileInfo(g_pFile->fileName()).absoluteFilePath());
        copyTextToClipboard(sFileName, tr("File path copied"));
    }
}

void GuiMainWindow::actionCopyFileNameSlot()
{
    if (g_pFile) {
        copyTextToClipboard(QFileInfo(g_pFile->fileName()).fileName(), tr("File name copied"));
    }
}

void GuiMainWindow::actionCopyFolderPathSlot()
{
    if (g_pFile) {
        const QString sFolderPath = QDir::toNativeSeparators(QFileInfo(g_pFile->fileName()).absolutePath());
        copyTextToClipboard(sFolderPath, tr("Folder path copied"));
    }
}

void GuiMainWindow::actionShowInFolderSlot()
{
    if (g_pFile) {
        XOptions::showInFolder(g_pFile->fileName());
    }
}

void GuiMainWindow::actionHexCommandSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (g_pHexView && pAction) {
        const QByteArray baMethod = pAction->property("HEX_METHOD").toByteArray();

        if (!baMethod.isEmpty()) {
            QMetaObject::invokeMethod(g_pHexView, baMethod.constData(), Qt::QueuedConnection);

            if (baMethod == QByteArrayLiteral("reloadView")) {
                updateFileStatus();
                statusBar()->showMessage(tr("View reloaded"), 2000);
            }
        }
    }
}

void GuiMainWindow::actionNavigateBackSlot()
{
    if (g_pHexView) {
        g_pHexView->goToPrevVisited();
        g_pHexView->setFocus();
        statusBar()->showMessage(tr("Previous visited location"), 2000);
    }
}

void GuiMainWindow::actionNavigateForwardSlot()
{
    if (g_pHexView) {
        g_pHexView->goToNextVisited();
        g_pHexView->setFocus();
        statusBar()->showMessage(tr("Next visited location"), 2000);
    }
}

void GuiMainWindow::actionGoToFileStartSlot()
{
    if (g_pHexView) {
        static_cast<XDeviceTableView *>(g_pHexView)->goToOffset(0, false, false, true);
        g_pHexView->setFocus();
        g_pHexView->viewport()->update();
        statusBar()->showMessage(tr("Moved to file start"), 2000);
    }
}

void GuiMainWindow::actionGoToFileEndSlot()
{
    if (g_pHexView && g_pFile) {
        const qint64 nLastOffset = qMax<qint64>(0, g_pFile->size() - 1);
        static_cast<XDeviceTableView *>(g_pHexView)->goToOffset(nLastOffset, false, false, true);
        g_pHexView->setFocus();
        g_pHexView->viewport()->update();
        statusBar()->showMessage(tr("Moved to file end"), 2000);
    }
}

void GuiMainWindow::actionQuickGoSlot()
{
    if (!g_pHexView || !g_pFile || !g_pQuickOffsetLineEdit) {
        return;
    }

    QString sOffset = g_pQuickOffsetLineEdit->text().trimmed();
    sOffset.remove(QLatin1Char('_'));
    sOffset.remove(QLatin1Char(' '));

    qint32 nBase = 16;
    if (sOffset.endsWith(QLatin1Char('d'), Qt::CaseInsensitive)) {
        nBase = 10;
        sOffset.chop(1);
    } else {
        if (sOffset.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive)) {
            sOffset.remove(0, 2);
        } else if (sOffset.startsWith(QLatin1Char('$'))) {
            sOffset.remove(0, 1);
        }
        if (sOffset.endsWith(QLatin1Char('h'), Qt::CaseInsensitive)) {
            sOffset.chop(1);
        }
    }

    bool bSuccess = false;
    const quint64 nOffset = sOffset.toULongLong(&bSuccess, nBase);
    const quint64 nMaximumOffset = g_pFile->size() > 0 ? (quint64)(g_pFile->size() - 1) : 0;

    if (!bSuccess || sOffset.isEmpty() || (nOffset > nMaximumOffset)) {
        g_pQuickOffsetLineEdit->setFocus();
        g_pQuickOffsetLineEdit->selectAll();
        statusBar()->showMessage(tr("Invalid offset (maximum 0x%1)").arg(QString::number(nMaximumOffset, 16).toUpper()), 3000);
        return;
    }

    static_cast<XDeviceTableView *>(g_pHexView)->goToOffset((qint64)nOffset, false, false, true);
    g_pHexView->setFocus();
    g_pHexView->viewport()->update();
    updateNavigationActions();
    updateQuickOffset();
    statusBar()->showMessage(tr("Moved to offset 0x%1").arg(QString::number(nOffset, 16).toUpper()), 2000);
}

void GuiMainWindow::actionElementModeSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        setElementMode((XHexView::ELEMENT_MODE)pAction->data().toInt());
    }
}

void GuiMainWindow::actionTextEncodingSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        setTextEncoding(pAction->data().toString());
    }
}

void GuiMainWindow::actionBytesPerLineSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        setBytesPerLine(pAction->data().toInt());
    }
}

void GuiMainWindow::actionLocationModeSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        setLocationMode((XBinaryView::LOCMODE)pAction->data().toInt());
    }
}

void GuiMainWindow::actionLocationBaseSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (pAction) {
        setLocationBase(pAction->data().toInt());
    }
}

void GuiMainWindow::actionCopyChecksumSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());

    if (!g_pFile || !pAction) {
        return;
    }

    QFile checksumFile(g_pFile->fileName());
    if (!checksumFile.open(QIODevice::ReadOnly)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot read the current file"));
        return;
    }

    QCryptographicHash hash((QCryptographicHash::Algorithm)pAction->data().toInt());
    if (!hash.addData(&checksumFile)) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot calculate the file checksum"));
        return;
    }

    const QString sName = pAction->property("HASH_NAME").toString();
    copyTextToClipboard(QString::fromLatin1(hash.result().toHex()), tr("%1 checksum copied").arg(sName));
}

void GuiMainWindow::actionCopySelectionFormattedSlot()
{
    QAction *pAction = qobject_cast<QAction *>(sender());
    QByteArray baData;

    if (!pAction || !readSelectedBytes(&baData)) {
        return;
    }

    const qint32 nFormat = pAction->data().toInt();
    QByteArray baResult;

    if (nFormat == 0) {
        baResult = baData.toHex(' ').toUpper();
    } else if (nFormat == 1) {
        baResult = baData.toHex().toUpper();
    } else {
        static const char cHexDigits[] = "0123456789ABCDEF";
        const bool bCArray = nFormat == 2;
        baResult.reserve(baData.size() * (bCArray ? 6 : 4) + 4);

        if (bCArray) {
            baResult.append("{ ");
        }

        for (qint32 i = 0; i < baData.size(); i++) {
            const quint8 nByte = (quint8)baData.at(i);

            if (bCArray) {
                if (i) {
                    baResult.append(", ");
                }
                baResult.append("0x");
            } else {
                baResult.append("\\x");
            }

            baResult.append(cHexDigits[(nByte >> 4) & 0x0F]);
            baResult.append(cHexDigits[nByte & 0x0F]);
        }

        if (bCArray) {
            baResult.append(" }");
        }
    }

    const QStringList listMessages = {tr("Spaced hexadecimal bytes copied"), tr("Compact hexadecimal string copied"), tr("C/C++ byte array copied"),
                                      tr("Escaped bytes copied")};
    copyTextToClipboard(QString::fromLatin1(baResult), listMessages.value(nFormat, tr("Selection copied")));
}

void GuiMainWindow::actionCopySelectionRangeSlot()
{
    if (!g_pHexView) {
        return;
    }

    const XDeviceTableView::DEVICESTATE state = g_pHexView->getDeviceState();
    if (state.nSelectionSize <= 0) {
        return;
    }

    const quint64 nStart = state.nSelectionDeviceOffset;
    const quint64 nEnd = nStart + (quint64)state.nSelectionSize - 1;
    const bool bHexadecimal = g_pHexView->getLocationBase() == 16;
    const auto formatLocation = [bHexadecimal](quint64 nValue) {
        return bHexadecimal ? QStringLiteral("0x%1").arg(QString::number(nValue, 16).toUpper()) : QString::number(nValue);
    };

    copyTextToClipboard(QStringLiteral("%1 - %2").arg(formatLocation(nStart), formatLocation(nEnd)), tr("Selection range copied"));
}

void GuiMainWindow::actionCopySelectionSizeSlot()
{
    if (!g_pHexView) {
        return;
    }

    const XDeviceTableView::DEVICESTATE state = g_pHexView->getDeviceState();
    if (state.nSelectionSize <= 0) {
        return;
    }

    const QString sSize = g_pHexView->getLocationBase() == 16 ? QStringLiteral("0x%1").arg(QString::number((quint64)state.nSelectionSize, 16).toUpper())
                                                              : QString::number(state.nSelectionSize);
    copyTextToClipboard(sSize, tr("Selection size copied"));
}

void GuiMainWindow::actionZoomInSlot()
{
    changeZoom(1);
}

void GuiMainWindow::actionZoomOutSlot()
{
    changeZoom(-1);
}

void GuiMainWindow::actionResetZoomSlot()
{
    if (!g_pHexView) {
        return;
    }

    QFont font;
    if (!font.fromString(g_xOptions.getValue(XOptions::ID_HEX_FONT).toString())) {
        font = XOptions::getMonoFont();
    }

    g_pHexView->setTextFont(font);
    statusBar()->showMessage(tr("Zoom reset: %1 pt").arg(font.pointSize()), 2000);
}

void GuiMainWindow::actionStayOnTopSlot(bool bChecked)
{
    g_xOptions.setValue(XOptions::ID_VIEW_STAYONTOP, bChecked);
    g_xOptions.adjustStayOnTop(this);
    statusBar()->showMessage(bChecked ? tr("Always on top enabled") : tr("Always on top disabled"), 2000);
}

void GuiMainWindow::actionOverviewMapSlot(bool bChecked)
{
    if (g_pHexView) {
        g_pHexView->setMapEnable(bChecked);
        g_pHexView->adjustView();
        statusBar()->showMessage(bChecked ? tr("Overview map shown") : tr("Overview map hidden"), 2000);
    }
}

void GuiMainWindow::actionResetDisplaySlot()
{
    setBytesPerLine(16, false);
    setElementMode(XHexView::ELEMENT_MODE_HEX, false);
    setTextEncoding(QString(), false);
    setLocationMode(XBinaryView::LOCMODE_OFFSET, false);
    setLocationBase(16, false);

    if (g_pHexView) {
        g_pHexView->setMapEnable(true);
        g_pHexView->adjustView();
    }

    syncDisplayActions();
    statusBar()->showMessage(tr("Display settings reset"), 2000);
}

void GuiMainWindow::selectionChangedSlot()
{
    const bool bHasSelection = g_pHexView && (g_pHexView->getDeviceState().nSelectionSize > 0);

    for (QAction *pAction : g_listSelectionActions) {
        pAction->setEnabled(bHasSelection);
    }

    updateQuickOffset();
}

void GuiMainWindow::viewWidgetsStateChangedSlot()
{
    for (QAction *pAction : g_listViewActions) {
        bool bChecked = false;

        if (g_pHexView) {
            const XDeviceTableEditView::VIEWWIDGET viewWidget = static_cast<XDeviceTableEditView::VIEWWIDGET>(pAction->property("VIEW_WIDGET").toInt());
            bChecked = g_pHexView->getViewWidgetState(viewWidget);
        }

        const QSignalBlocker signalBlocker(pAction);
        pAction->setChecked(bChecked);
    }
}

void GuiMainWindow::updateRecentFilesState()
{
    ui->toolButtonRecentWelcome->setEnabled(g_xOptions.getRecentFiles().count() > 0);
}

void GuiMainWindow::updateFileStatus()
{
    if (g_pFile) {
        const QFileInfo fileInfo(g_pFile->fileName());
        g_pFileStatus->setText(tr("%1 | %2 bytes | Read-only view | %3 bytes/line").arg(fileInfo.fileName(), QLocale().toString(fileInfo.size())).arg(g_nBytesPerLine));
    }
}

void GuiMainWindow::updateQuickOffset()
{
    if (!g_pQuickOffsetLineEdit) {
        return;
    }

    const bool bHasDocument = g_pHexView && g_pFile;
    g_pQuickOffsetLineEdit->setEnabled(bHasDocument);

    if (!bHasDocument) {
        g_pQuickOffsetLineEdit->clear();
        return;
    }

    if (!g_pQuickOffsetLineEdit->hasFocus()) {
        const qint64 nOffset = g_pHexView->getDeviceState().nSelectionDeviceOffset;
        if (nOffset >= 0) {
            g_pQuickOffsetLineEdit->setText(QStringLiteral("0x%1").arg(QString::number(nOffset, 16).toUpper()));
        }
    }
}

void GuiMainWindow::setBytesPerLine(qint32 nBytesPerLine, bool bShowMessage)
{
    if (!g_pHexView || (nBytesPerLine <= 0)) {
        return;
    }

    g_nBytesPerLine = nBytesPerLine;
    g_pHexView->setBytesProLine(nBytesPerLine);
    syncDisplayActions();
    updateFileStatus();

    if (bShowMessage) {
        statusBar()->showMessage(tr("%1 bytes per line").arg(nBytesPerLine), 2000);
    }
}

void GuiMainWindow::setElementMode(XHexView::ELEMENT_MODE mode, bool bShowMessage)
{
    if (!g_pHexView) {
        return;
    }

    g_elementMode = mode;
    g_pHexView->setElementMode(mode);
    syncDisplayActions();

    if (bShowMessage) {
        QString sMode;
        for (QAction *pAction : g_listElementModeActions) {
            if (pAction->data().toInt() == (qint32)mode) {
                sMode = pAction->text();
                break;
            }
        }
        statusBar()->showMessage(tr("Element format: %1").arg(sMode), 2000);
    }
}

void GuiMainWindow::setTextEncoding(const QString &sCodePage, bool bShowMessage)
{
    if (!g_pHexView) {
        return;
    }

    g_sTextEncoding = sCodePage;
    g_pHexView->setCodePage(sCodePage);
    syncDisplayActions();

    if (bShowMessage) {
        statusBar()->showMessage(tr("Text encoding: %1").arg(sCodePage.isEmpty() ? tr("System / default") : sCodePage), 2000);
    }
}

void GuiMainWindow::setLocationMode(XBinaryView::LOCMODE mode, bool bShowMessage)
{
    if (!g_pHexView) {
        return;
    }

    g_pHexView->setLocationMode(mode);
    syncDisplayActions();

    if (bShowMessage) {
        const QString sMode = (mode == XBinaryView::LOCMODE_ADDRESS) ? tr("Virtual addresses") : tr("File offsets");
        statusBar()->showMessage(tr("Location labels: %1").arg(sMode), 2000);
    }
}

void GuiMainWindow::setLocationBase(qint32 nBase, bool bShowMessage)
{
    if (!g_pHexView || ((nBase != 10) && (nBase != 16))) {
        return;
    }

    g_pHexView->setLocationBase(nBase);
    syncDisplayActions();

    if (bShowMessage) {
        statusBar()->showMessage(nBase == 16 ? tr("Location base: hexadecimal") : tr("Location base: decimal"), 2000);
    }
}

void GuiMainWindow::changeZoom(qint32 nDelta)
{
    if (!g_pHexView) {
        return;
    }

    QFont font = g_pHexView->getTextFont();
    qint32 nPointSize = font.pointSize();
    if (nPointSize <= 0) {
        nPointSize = XOptions::getMonoFont().pointSize();
    }

    nPointSize = qBound<qint32>(6, nPointSize + nDelta, 48);
    font.setPointSize(nPointSize);
    g_pHexView->setTextFont(font);
    statusBar()->showMessage(tr("Zoom: %1 pt").arg(nPointSize), 2000);
}

void GuiMainWindow::syncDisplayActions()
{
    for (QAction *pAction : g_listBytesPerLineActions) {
        const QSignalBlocker signalBlocker(pAction);
        pAction->setChecked(pAction->data().toInt() == g_nBytesPerLine);
    }

    if (g_pActionOverviewMap) {
        const QSignalBlocker signalBlocker(g_pActionOverviewMap);
        g_pActionOverviewMap->setChecked(g_pHexView && g_pHexView->isMapEnable());
    }

    if (g_pBytesPerLineCombo) {
        const QSignalBlocker signalBlocker(g_pBytesPerLineCombo);
        const qint32 nIndex = g_pBytesPerLineCombo->findData(g_nBytesPerLine);
        if (nIndex >= 0) {
            g_pBytesPerLineCombo->setCurrentIndex(nIndex);
        }
    }

    for (QAction *pAction : g_listElementModeActions) {
        const QSignalBlocker signalBlocker(pAction);
        pAction->setChecked(pAction->data().toInt() == (qint32)g_elementMode);
    }

    if (g_pElementModeCombo) {
        const QSignalBlocker signalBlocker(g_pElementModeCombo);
        const qint32 nIndex = g_pElementModeCombo->findData((qint32)g_elementMode);
        if (nIndex >= 0) {
            g_pElementModeCombo->setCurrentIndex(nIndex);
        }
    }

    for (QAction *pAction : g_listTextEncodingActions) {
        const QSignalBlocker signalBlocker(pAction);
        pAction->setChecked(pAction->data().toString().compare(g_sTextEncoding, Qt::CaseInsensitive) == 0);
    }

    const qint32 nLocationMode = g_pHexView ? (qint32)g_pHexView->getlocationMode() : -1;
    for (QAction *pAction : g_listLocationModeActions) {
        const QSignalBlocker signalBlocker(pAction);
        pAction->setChecked(pAction->data().toInt() == nLocationMode);
    }

    const qint32 nLocationBase = g_pHexView ? g_pHexView->getLocationBase() : -1;
    for (QAction *pAction : g_listLocationBaseActions) {
        const QSignalBlocker signalBlocker(pAction);
        pAction->setChecked(pAction->data().toInt() == nLocationBase);
    }
}

void GuiMainWindow::updateNavigationActions()
{
    if (g_pActionNavigateBack) {
        g_pActionNavigateBack->setEnabled(g_pHexView && g_pHexView->isPrevVisitedAvailable());
    }

    if (g_pActionNavigateForward) {
        g_pActionNavigateForward->setEnabled(g_pHexView && g_pHexView->isNextVisitedAvailable());
    }
}

void GuiMainWindow::adjustWindow()
{
    // ui->widgetViewer->adjustView();

    g_xOptions.adjustWindow(this);

    if (g_pActionStayOnTop) {
        const QSignalBlocker signalBlocker(g_pActionStayOnTop);
        g_pActionStayOnTop->setChecked(g_xOptions.isStayOnTop());
    }

    if (qApp->styleSheet().isEmpty()) {
        QFile styleFile(QStringLiteral(":/styles/xhexviewer.qss"));

        if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            setStyleSheet(QString::fromUtf8(styleFile.readAll()));
        }
    } else {
        setStyleSheet(QString());
    }

    // if (g_xOptions.isShowLogo()) {
    //     ui->labelLogo->show();
    // } else {
    //     ui->labelLogo->hide();
    // }
}

void GuiMainWindow::processFile(const QString &sFileName)
{
    if (sFileName.isEmpty() || !QFileInfo(sFileName).isFile()) {
        QMessageBox::critical(this, tr("Error"), tr("Cannot open file"));
        return;
    }

    QFile *pNewFile = new QFile(sFileName);
    if (!pNewFile->open(QIODevice::ReadOnly)) {
        delete pNewFile;
        QMessageBox::critical(this, tr("Error"), tr("Cannot open file"));
        return;
    }

    XBinary xbinary(pNewFile);
    if (!xbinary.isValid()) {
        pNewFile->close();
        delete pNewFile;
        QMessageBox::critical(this, tr("Error"), tr("It is not a valid file"));
        return;
    }

    closeCurrentFile();
    g_pFile = pNewFile;

    g_pXInfo = new XInfoDB;
    g_pXInfo->setData(g_pFile, xbinary.getFileType());
    g_pInfoMenu->setData(g_pXInfo);
    g_pInfoMenu->tryToLoad();

    const QFileInfo fileInfo(sFileName);

    XHexViewWidget::OPTIONS options = {};
    XHexViewWidget *pHexViewWidget = new XHexViewWidget;
    pHexViewWidget->setObjectName(QStringLiteral("hexWorkspace"));
    pHexViewWidget->setWindowTitle(fileInfo.fileName());
    pHexViewWidget->setGlobal(&g_xShortcuts, &g_xOptions);
    pHexViewWidget->setData(g_pFile, options);
    pHexViewWidget->setXInfoDB(g_pXInfo);
    pHexViewWidget->reload();
    g_pHexView = pHexViewWidget->findChild<XHexViewEx *>(QStringLiteral("scrollAreaHex"));
    g_nBytesPerLine = 16;
    g_elementMode = XHexView::ELEMENT_MODE_HEX;
    g_sTextEncoding.clear();

    if (g_pHexView) {
        g_pHexView->setBytesProLine(g_nBytesPerLine);
        g_pHexView->setMapEnable(true);
        g_elementMode = g_pHexView->getElementMode();
        g_sTextEncoding = g_pHexView->getCodePage();
        connect(g_pHexView, SIGNAL(selectionChanged()), this, SLOT(selectionChangedSlot()));
        connect(g_pHexView, SIGNAL(viewWidgetsStateChanged()), this, SLOT(viewWidgetsStateChangedSlot()));
        connect(g_pHexView, &XDeviceTableView::visitedStateChanged, this, [this]() { updateNavigationActions(); });
        connect(g_pHexView, &XHexView::elementModeChanged, this, [this](qint32 nMode) {
            g_elementMode = (XHexView::ELEMENT_MODE)nMode;
            syncDisplayActions();
        });
        connect(g_pHexView, &XHexView::codePageChanged, this, [this](const QString &sCodePage) {
            g_sTextEncoding = sCodePage;
            syncDisplayActions();
        });
        connect(g_pHexView, &XDeviceTableView::locationModeChanged, this, [this](qint32) { syncDisplayActions(); });
        connect(g_pHexView, &XDeviceTableView::locationBaseChanged, this, [this](qint32) { syncDisplayActions(); });
    }

    QVBoxLayout *pWorkspaceLayout = qobject_cast<QVBoxLayout *>(pHexViewWidget->layout());
    QHBoxLayout *pControlLayout = pHexViewWidget->findChild<QHBoxLayout *>(QStringLiteral("horizontalLayout"));
    QHBoxLayout *pBaseLayout = pHexViewWidget->findChild<QHBoxLayout *>(QStringLiteral("horizontalLayout_2"));
    QComboBox *pTypeComboBox = pHexViewWidget->findChild<QComboBox *>(QStringLiteral("comboBoxType"));
    QComboBox *pBaseComboBox = pHexViewWidget->findChild<QComboBox *>(QStringLiteral("comboBoxLocationBase"));
    QToolButton *pInspectorButton = pHexViewWidget->findChild<QToolButton *>(QStringLiteral("toolButtonDataInspector"));
    QToolButton *pStringsButton = pHexViewWidget->findChild<QToolButton *>(QStringLiteral("toolButtonStrings"));

    if (pInspectorButton) {
        pInspectorButton->hide();
    }
    if (pStringsButton) {
        pStringsButton->hide();
    }

    if (pWorkspaceLayout && pControlLayout && pBaseLayout && pTypeComboBox && pBaseComboBox) {
        QLabel *pFormatLabel = new QLabel(tr("Format"), pHexViewWidget);
        QLabel *pBaseLabel = new QLabel(tr("Base"), pHexViewWidget);
        QLabel *pBytesPerLineLabel = new QLabel(tr("Bytes/line"), pHexViewWidget);
        QLabel *pElementModeLabel = new QLabel(tr("Elements"), pHexViewWidget);
        QComboBox *pBytesPerLineCombo = new QComboBox(pHexViewWidget);
        QComboBox *pElementModeCombo = new QComboBox(pHexViewWidget);
        pFormatLabel->setObjectName(QStringLiteral("hexToolbarLabel"));
        pBaseLabel->setObjectName(QStringLiteral("hexToolbarLabel"));
        pBytesPerLineLabel->setObjectName(QStringLiteral("hexToolbarLabel"));
        pElementModeLabel->setObjectName(QStringLiteral("hexToolbarLabel"));
        pBytesPerLineCombo->setObjectName(QStringLiteral("comboBoxBytesPerLine"));
        pElementModeCombo->setObjectName(QStringLiteral("comboBoxElementMode"));
        for (qint32 nBytesPerLine : QList<qint32>({8, 16, 24, 32, 48, 64})) {
            pBytesPerLineCombo->addItem(QString::number(nBytesPerLine), nBytesPerLine);
        }
        for (QAction *pAction : g_listElementModeActions) {
            pElementModeCombo->addItem(pAction->property("COMBO_TEXT").toString(), pAction->data());
        }
        pTypeComboBox->setMinimumWidth(150);
        pTypeComboBox->setMaximumWidth(220);
        pBaseComboBox->setMinimumWidth(72);
        pBaseComboBox->setMaximumWidth(90);
        pBytesPerLineCombo->setMinimumWidth(72);
        pBytesPerLineCombo->setMaximumWidth(90);
        pElementModeCombo->setMinimumWidth(82);
        pElementModeCombo->setMaximumWidth(100);
        pControlLayout->setSpacing(8);
        pControlLayout->insertWidget(0, pFormatLabel);
        pControlLayout->insertWidget(2, pBaseLabel);
        pControlLayout->insertWidget(3, pBaseComboBox);
        pControlLayout->insertWidget(4, pBytesPerLineLabel);
        pControlLayout->insertWidget(5, pBytesPerLineCombo);
        pControlLayout->insertWidget(6, pElementModeLabel);
        pControlLayout->insertWidget(7, pElementModeCombo);
        pWorkspaceLayout->removeItem(pBaseLayout);
        delete pBaseLayout;
        pWorkspaceLayout->setContentsMargins(12, 10, 12, 8);
        pWorkspaceLayout->setSpacing(8);

        g_pBytesPerLineCombo = pBytesPerLineCombo;
        g_pElementModeCombo = pElementModeCombo;
        connect(pBytesPerLineCombo, QOverload<int>::of(&QComboBox::activated), this,
                [this, pBytesPerLineCombo](qint32 nIndex) { setBytesPerLine(pBytesPerLineCombo->itemData(nIndex).toInt()); });
        connect(pElementModeCombo, QOverload<int>::of(&QComboBox::activated), this,
                [this, pElementModeCombo](qint32 nIndex) { setElementMode((XHexView::ELEMENT_MODE)pElementModeCombo->itemData(nIndex).toInt()); });
        syncDisplayActions();
    }

    QMdiSubWindow *pSubWindow = ui->mdiArea->addSubWindow(pHexViewWidget, Qt::FramelessWindowHint);
    pSubWindow->setAttribute(Qt::WA_DeleteOnClose);
    pSubWindow->showMaximized();
    pHexViewWidget->setWidgetFocus();
    ui->stackedWidget->setCurrentWidget(ui->pageViewer);
    g_pActionClose->setEnabled(true);
    setDocumentActionsEnabled(g_pHexView != nullptr);
    updateQuickOffset();

    g_xOptions.setLastFileName(sFileName);
    updateRecentFilesState();
    adjustWindow();
    updateFileStatus();
    setWindowTitle(QStringLiteral("%1 - %2").arg(fileInfo.fileName(), XOptions::getTitle(X_APPLICATIONDISPLAYNAME, X_APPLICATIONVERSION, false)));
}

void GuiMainWindow::errorMessageSlot(const QString &sText)
{
    QMessageBox::critical(this, tr("Error"), sText);
}

void GuiMainWindow::closeCurrentFile()
{
    setDocumentActionsEnabled(false);
    g_pHexView = nullptr;
    g_pBytesPerLineCombo = nullptr;
    g_pElementModeCombo = nullptr;
    if (g_pQuickOffsetLineEdit) {
        g_pQuickOffsetLineEdit->setEnabled(false);
        g_pQuickOffsetLineEdit->clear();
    }
    g_nBytesPerLine = 16;
    g_elementMode = XHexView::ELEMENT_MODE_HEX;
    g_sTextEncoding.clear();
    ui->mdiArea->closeAllSubWindows();

    if (g_pXInfo) {
        g_pInfoMenu->tryToSave();
        g_pInfoMenu->reset();
        delete g_pXInfo;
        g_pXInfo = nullptr;
    }

    if (g_pFile) {
        g_pFile->close();
        delete g_pFile;
        g_pFile = nullptr;
    }

    // ui->stackedWidget->setCurrentIndex(0);
    // ui->widgetViewer->cleanup();

    if (g_pActionClose) {
        g_pActionClose->setEnabled(false);
    }

    ui->stackedWidget->setCurrentWidget(ui->pageWelcome);
    g_pFileStatus->setText(tr("Ready - open or drop a binary file"));
    setWindowTitle(XOptions::getTitle(X_APPLICATIONDISPLAYNAME, X_APPLICATIONVERSION, false));
}

void GuiMainWindow::dragEnterEvent(QDragEnterEvent *pEvent)
{
    const QList<QUrl> urlList = pEvent->mimeData()->urls();

    for (const QUrl &url : urlList) {
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile()) {
            pEvent->acceptProposedAction();
            return;
        }
    }

    pEvent->ignore();
}

void GuiMainWindow::dragMoveEvent(QDragMoveEvent *pEvent)
{
    const QList<QUrl> urlList = pEvent->mimeData()->urls();

    for (const QUrl &url : urlList) {
        if (url.isLocalFile() && QFileInfo(url.toLocalFile()).isFile()) {
            pEvent->acceptProposedAction();
            return;
        }
    }

    pEvent->ignore();
}

void GuiMainWindow::dropEvent(QDropEvent *pEvent)
{
    const QMimeData *mimeData = pEvent->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        for (const QUrl &url : urlList) {
            if (!url.isLocalFile() || !QFileInfo(url.toLocalFile()).isFile()) {
                continue;
            }

            QString sFileName = url.toLocalFile();

            sFileName = XBinary::convertFileName(sFileName);

            processFile(sFileName);
            pEvent->acceptProposedAction();
            return;
        }
    }

    pEvent->ignore();
}

void GuiMainWindow::actionShortcutsSlot()
{
    DialogShortcuts dialogShortcuts(this);

    dialogShortcuts.setData(&g_xShortcuts);

    dialogShortcuts.exec();

    updateMenuShortcutHints();
    adjustWindow();
}

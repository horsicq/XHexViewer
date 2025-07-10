/* Copyright (c) 2019-2025 hors<horsicq@gmail.com>
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

#include "ui_guimainwindow.h"

GuiMainWindow::GuiMainWindow(QWidget *pParent) : QMainWindow(pParent), ui(new Ui::GuiMainWindow)
{
    ui->setupUi(this);

    // ui->mdiArea->setDocumentMode(false);
    ui->mdiArea->setViewMode(QMdiArea::TabbedView);

    g_pFile = nullptr;
    g_pXInfo = nullptr;

    setWindowTitle(XOptions::getTitle(X_APPLICATIONDISPLAYNAME, X_APPLICATIONVERSION));

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

    delete ui;
}

void GuiMainWindow::createMenus()
{
    QMenu *pMenuFile = new QMenu(tr("File"), ui->menubar);
    QMenu *pMenuTools = new QMenu(tr("Tools"), ui->menubar);
    QMenu *pMenuHelp = new QMenu(tr("Help"), ui->menubar);

    ui->menubar->addAction(pMenuFile->menuAction());
    ui->menubar->addAction(pMenuTools->menuAction());
    ui->menubar->addAction(pMenuHelp->menuAction());

    QAction *pActionOpen = new QAction(tr("Open"), this);
    QAction *pActionClose = new QAction(tr("Close"), this);
    QAction *pActionExit = new QAction(tr("Exit"), this);
    QAction *pActionOptions = new QAction(tr("Options"), this);
    QAction *pActionAbout = new QAction(tr("About"), this);
    QAction *pActionShortcuts = new QAction(tr("Shortcuts"), this);
    QAction *pActionDemangle = new QAction(tr("Demangle"), this);

    pMenuFile->addAction(pActionOpen);
    pMenuFile->addMenu(g_xOptions.createRecentFilesMenu(this));
    pMenuFile->addMenu(g_pInfoMenu->createMenu(this));
    pMenuFile->addAction(pActionClose);
    pMenuFile->addAction(pActionExit);
    pMenuTools->addAction(pActionDemangle);
    pMenuTools->addAction(pActionShortcuts);
    pMenuTools->addAction(pActionOptions);
    pMenuHelp->addAction(pActionAbout);

    connect(pActionOpen, SIGNAL(triggered()), this, SLOT(actionOpenSlot()));
    connect(pActionClose, SIGNAL(triggered()), this, SLOT(actionCloseSlot()));
    connect(pActionExit, SIGNAL(triggered()), this, SLOT(actionExitSlot()));
    connect(pActionOptions, SIGNAL(triggered()), this, SLOT(actionOptionsSlot()));
    connect(pActionAbout, SIGNAL(triggered()), this, SLOT(actionAboutSlot()));
    connect(pActionShortcuts, SIGNAL(triggered()), this, SLOT(actionShortcutsSlot()));
    connect(pActionDemangle, SIGNAL(triggered()), this, SLOT(actionDemangleSlot()));
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

void GuiMainWindow::adjustWindow()
{
    // ui->widgetViewer->adjustView();

    g_xOptions.adjustWindow(this);

    // if (g_xOptions.isShowLogo()) {
    //     ui->labelLogo->show();
    // } else {
    //     ui->labelLogo->hide();
    // }
}

void GuiMainWindow::processFile(const QString &sFileName)
{
    if ((sFileName != "") && (QFileInfo(sFileName).isFile())) {
        g_xOptions.setLastFileName(sFileName);

        closeCurrentFile();

        QFile *pFile = new QFile;
        XInfoDB *pXInfo = new XInfoDB;

        pFile->setFileName(sFileName);

        if (!pFile->open(QIODevice::ReadWrite)) {
            if (!pFile->open(QIODevice::ReadOnly)) {
                closeCurrentFile();
            }
        }

        if (pFile) {
            XBinary xbinary(pFile);
            if (xbinary.isValid()) {
                pXInfo->setData(pFile, xbinary.getFileType());
                g_pInfoMenu->setData(pXInfo);
                g_pInfoMenu->tryToLoad();

                XHexViewWidget::OPTIONS options = {};

                XHexViewWidget *pHexViewWidget = new XHexViewWidget;
                pHexViewWidget->setGlobal(&g_xShortcuts, &g_xOptions);
                pHexViewWidget->setData(pFile, options);
                pHexViewWidget->setXInfoDB(pXInfo);
                pHexViewWidget->reload();

                ui->mdiArea->addSubWindow(pHexViewWidget);
                pHexViewWidget->show();

                // ui->widgetViewer->setData(g_pFile, options);
                // ui->widgetViewer->setXInfoDB(g_pXInfo);

                // ui->widgetViewer->reload();

                adjustWindow();

                setWindowTitle(sFileName);
                // ui->stackedWidget->setCurrentIndex(1);
            } else {
                QMessageBox::critical(this, tr("Error"), tr("It is not a valid file"));
            }
        } else {
            QMessageBox::critical(this, tr("Error"), tr("Cannot open file"));
        }
    } else {
        QMessageBox::critical(this, tr("Error"), tr("Cannot open file"));
    }
}

void GuiMainWindow::errorMessageSlot(const QString &sText)
{
    QMessageBox::critical(this, tr("Error"), sText);
}

void GuiMainWindow::closeCurrentFile()
{
    if (g_pXInfo) {
        g_pInfoMenu->tryToSave();
        delete g_pXInfo;
        g_pXInfo = nullptr;
        g_pInfoMenu->reset();
    }

    if (g_pFile) {
        g_pFile->close();
        delete g_pFile;
        g_pFile = nullptr;
    }

    // ui->stackedWidget->setCurrentIndex(0);
    // ui->widgetViewer->cleanup();

    setWindowTitle(XOptions::getTitle(X_APPLICATIONDISPLAYNAME, X_APPLICATIONVERSION));
}

void GuiMainWindow::dragEnterEvent(QDragEnterEvent *pEvent)
{
    pEvent->acceptProposedAction();
}

void GuiMainWindow::dragMoveEvent(QDragMoveEvent *pEvent)
{
    pEvent->acceptProposedAction();
}

void GuiMainWindow::dropEvent(QDropEvent *pEvent)
{
    const QMimeData *mimeData = pEvent->mimeData();

    if (mimeData->hasUrls()) {
        QList<QUrl> urlList = mimeData->urls();

        if (urlList.count()) {
            QString sFileName = urlList.at(0).toLocalFile();

            sFileName = XBinary::convertFileName(sFileName);

            processFile(sFileName);
        }
    }
}

void GuiMainWindow::actionShortcutsSlot()
{
    DialogShortcuts dialogShortcuts(this);

    dialogShortcuts.setData(&g_xShortcuts);

    dialogShortcuts.exec();

    adjustWindow();
}

void GuiMainWindow::actionDemangleSlot()
{
    //    DialogDemangle dialogDemangle(this);

    //    dialogDemangle.exec();
}

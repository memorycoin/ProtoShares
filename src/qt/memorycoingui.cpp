// Copyright (c) 2010 Satoshi Nakamoto
// Copyright (c) 2009-2012 The Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
// Copyright (c) 2013-2014 Memorycoin Dev Team
/*
 * Qt4 memorycoin GUI.
 *
 * W.J. van der Laan 2011-2012
 * The Memorycoin Developers 2011-2012
 */

#include <QApplication>
#include <QUrl>
#include <QDesktopServices>
#include <QtGlobal>

#include "memorycoingui.h"

#include "transactiontablemodel.h"
#include "optionsdialog.h"
#include "aboutdialog.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "walletframe.h"
#include "optionsmodel.h"
#include "transactiondescdialog.h"
#include "memorycoinunits.h"
#include "guiconstants.h"
#include "notificator.h"
#include "guiutil.h"
#include "rpcconsole.h"
#include "ui_interface.h"
#include "wallet.h"
#include "init.h"
#include "memorycoinrpc.h"

#ifdef Q_OS_MAC
#include "macdockiconhandler.h"
#endif

#include <QMenuBar>
#include <QMenu>
#include <QIcon>
#include <QVBoxLayout>
#include <QToolBar>
#include <QStatusBar>
#include <QLabel>
#include <QMessageBox>
#include <QProgressBar>
#include <QStackedWidget>
#include <QDateTime>
#include <QMovie>
#include <QTimer>
#include <QDragEnterEvent>
#include <QUrl>
#include <QMimeData>
#include <QStyle>
#include <QSettings>
#include <QDesktopWidget>
#include <QListWidget>

#include <iostream>

const QString MemorycoinGUI::DEFAULT_WALLET = "~Default";

MemorycoinGUI::MemorycoinGUI(QWidget *parent) :
    QMainWindow(parent),
    clientModel(0),
    encryptWalletAction(0),
    changePassphraseAction(0),
    miningOffAction(0),
    miningOneAction(0),
    miningTwoAction(0),
    miningThreeAction(0),
    miningFourAction(0),
    miningFiveAction(0),
    miningSixAction(0),
    miningPoolAESONAction(0),
    miningPoolAESOFFAction(0),
    balanceMMCAction(0),
    balanceDwarfAction(0),
    balance1GHAction(0),
    currentVotesAction(0),
    currentCandidatesAction(0),
    howToVoteAction(0),
    currentResultsAction(0),
    aboutQtAction(0),
    trayIcon(0),
    notificator(0),
    rpcConsole(0),
    prevBlocks(0)
{
    restoreWindowGeometry();
    setWindowTitle(tr("MemoryCoin") + " - " + tr("Wallet"));
#ifndef Q_OS_MAC
    QApplication::setWindowIcon(QIcon(":icons/memorycoin"));
    setWindowIcon(QIcon(":icons/memorycoin"));
#else
    setUnifiedTitleAndToolBarOnMac(true);
    QApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
#endif
    // Create wallet frame and make it the central widget
    walletFrame = new WalletFrame(this);
    setCentralWidget(walletFrame);

    // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon();

    // Create status bar
    statusBar();

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setMinimumWidth(83);
    frameBlocks->setMaximumWidth(83);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    labelEncryptionIcon = new QLabel();
    labelMiningIcon = new QLabel();
    labelConnectionsIcon = new QLabel();
    labelBlocksIcon = new QLabel();
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelEncryptionIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelMiningIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelConnectionsIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new QProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setVisible(false);

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://qt-project.org/doc/qt-4.8/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(progressBarLabel);
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    syncIconMovie = new QMovie(":/movies/update_spinner", "mng", this);

    rpcConsole = new RPCConsole(this);
    connect(openRPCConsoleAction, SIGNAL(triggered()), rpcConsole, SLOT(show()));

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);
}

MemorycoinGUI::~MemorycoinGUI()
{
    saveWindowGeometry();
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
    MacDockIconHandler::instance()->setMainWindow(NULL);
#endif
}

void MemorycoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(QIcon(":/icons/overview"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(QIcon(":/icons/send"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a MemoryCoin address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    receiveCoinsAction = new QAction(QIcon(":/icons/receiving_addresses"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Show the list of addresses for receiving payments"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    historyAction = new QAction(QIcon(":/icons/history"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

    addressBookAction = new QAction(QIcon(":/icons/address-book"), tr("&Addresses"), this);
    addressBookAction->setStatusTip(tr("Edit the list of stored addresses and labels"));
    addressBookAction->setToolTip(addressBookAction->statusTip());
    addressBookAction->setCheckable(true);
    addressBookAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
    tabGroup->addAction(addressBookAction);

    voteCoinsAction = new QAction(QIcon(":/icons/voting_prefs"), tr("&Vote"), this);
    voteCoinsAction->setStatusTip(tr("Vote in MemoryCoin elections"));
    voteCoinsAction->setToolTip(voteCoinsAction->statusTip());
    voteCoinsAction->setCheckable(true);
    voteCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_6));
    tabGroup->addAction(voteCoinsAction);


    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(addressBookAction, SIGNAL(triggered()), this, SLOT(gotoAddressBookPage()));
    connect(voteCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(voteCoinsAction, SIGNAL(triggered()), this, SLOT(gotoVoteCoinsPage()));

    quitAction = new QAction(QIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(QIcon(":/icons/memorycoin"), tr("&About MemoryCoin"), this);
    aboutAction->setStatusTip(tr("Show information about MemoryCoin"));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutQtAction = new QAction(QIcon(":/trolltech/qmessagebox/images/qtlogo-64.png"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(QIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for MemoryCoin"));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    toggleHideAction = new QAction(QIcon(":/icons/memorycoin"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(QIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(QIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(QIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));
    
    miningOffAction = new QAction(QIcon(":/icons/mining_inactive"), tr("Solo Mining Off"), this);
    miningOffAction->setStatusTip(tr("Stop Mining. May take some time to wind down."));
    miningOneAction = new QAction(QIcon(":/icons/mining_active"), tr("Solo Mining On (1GB Required)"), this);
    miningOneAction->setStatusTip(tr("Mine MemoryCoin solo - not recommended!"));
    miningTwoAction = new QAction(QIcon(":/icons/mining"), tr("Mine 2 Processes (1GB Required)"), this);
    miningTwoAction->setStatusTip(tr("Mine MemoryCoin with 2 processes. 1GB Required."));
    miningThreeAction = new QAction(QIcon(":/icons/mining"), tr("Mine 4 Processes (1GB Required)"), this);
    miningThreeAction->setStatusTip(tr("Mine MemoryCoin with 4 processes. 1GB Required."));
    miningFourAction = new QAction(QIcon(":/icons/mining"), tr("Mine 8 Processes (1GB Required)"), this);
    miningFourAction->setStatusTip(tr("Mine MemoryCoin with 8 processes. 1GB Required."));
    miningFiveAction = new QAction(QIcon(":/icons/mining"), tr("Mine 16 Processes (1GB Required)"), this);
    miningFiveAction->setStatusTip(tr("Mine MemoryCoin with 16 processes. 1GB Required."));
    miningSixAction = new QAction(QIcon(":/icons/mining"), tr("Mine 32 Processes (1GB Required)"), this);
    miningSixAction->setStatusTip(tr("Mine MemoryCoin with 32 processes. 1GB Required."));
    miningPoolAESONAction = new QAction(QIcon(":/icons/mining_active"), tr("Launch Pool Miner"), this);
    miningPoolAESONAction->setStatusTip(tr("Launch Pool Miner ((64 bit / 1GB required))"));
    miningPoolAESOFFAction = new QAction(QIcon(":/icons/mining_active"), tr("Launch Pool Miner"), this);
    miningPoolAESOFFAction->setStatusTip(tr("Launch Pool Miner"));

    balanceMMCAction = new QAction(QIcon(":/icons/coin"), tr("Pool Balance (MMCPool)"), this);
    balanceMMCAction->setStatusTip(tr("Pool Balance (MMCPool))"));
    balanceDwarfAction = new QAction(QIcon(":/icons/coin"), tr("Pool Balance (Dwarf Pool)"), this);
    balanceDwarfAction->setStatusTip(tr("Pool Balance (Dwarf Pool))"));
    balance1GHAction = new QAction(QIcon(":/icons/coin"), tr("Pool Balance (1GH Pool)"), this);
    balance1GHAction->setStatusTip(tr("Pool Balance (1GH Pool))"));

    currentVotesAction = new QAction(QIcon(":/icons/voting_prefs"), tr("Current Voting Preferences"), this);
    currentVotesAction->setStatusTip(tr("Show who or what you are currently voting for. (Web)"));
    currentCandidatesAction = new QAction(QIcon(":/icons/voting_candidates"), tr("Current Candidates"), this);
    currentCandidatesAction->setStatusTip(tr("Display a list of current candidates. (Web)"));
    howToVoteAction = new QAction(QIcon(":/icons/voting_how"), tr("How To Vote"), this);
    howToVoteAction->setStatusTip(tr("Information about how to cast your vote. (Web)"));
    currentResultsAction = new QAction(QIcon(":/icons/voting_results"), tr("Latest Election Results"), this);
    currentResultsAction->setStatusTip(tr("Current board and past results. (Web)"));    
   
    
    signMessageAction = new QAction(QIcon(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your MemoryCoin addresses to prove you own them"));
    verifyMessageAction = new QAction(QIcon(":/icons/transaction_0"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified MemoryCoin addresses"));

    openRPCConsoleAction = new QAction(QIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));
#ifdef Q_OS_MAC
    createMenuBar();
#endif
    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
    connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
    connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));
    connect(miningOffAction, SIGNAL(triggered()), this, SLOT(miningOff()));
    connect(miningOneAction, SIGNAL(triggered()), this, SLOT(miningOne()));
    connect(miningTwoAction, SIGNAL(triggered()), this, SLOT(miningTwo()));
    connect(miningThreeAction, SIGNAL(triggered()), this, SLOT(miningThree()));
    connect(miningFourAction, SIGNAL(triggered()), this, SLOT(miningFour()));
    connect(miningFiveAction, SIGNAL(triggered()), this, SLOT(miningFive()));
    connect(miningSixAction, SIGNAL(triggered()), this, SLOT(miningSix()));
    connect(miningPoolAESONAction, SIGNAL(triggered()), this, SLOT(miningPoolAESON()));
    connect(miningPoolAESOFFAction, SIGNAL(triggered()), this, SLOT(miningPoolAESOFF()));
    
    connect(balanceMMCAction, SIGNAL(triggered()), this, SLOT(balanceMMC()));
    connect(balanceDwarfAction, SIGNAL(triggered()), this, SLOT(balanceDwarf()));
    connect(balance1GHAction, SIGNAL(triggered()), this, SLOT(balance1GH()));

    connect(currentVotesAction, SIGNAL(triggered()), this, SLOT(currentVotes()));    
    connect(currentCandidatesAction, SIGNAL(triggered()), this, SLOT(currentCandidates()));    
    connect(howToVoteAction, SIGNAL(triggered()), this, SLOT(howToVote()));    
    connect(currentResultsAction, SIGNAL(triggered()), this, SLOT(currentResults()));    
    
    connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
    connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
}

void MemorycoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    file->addAction(backupWalletAction);
    file->addAction(signMessageAction);
    file->addAction(verifyMessageAction);
    file->addSeparator();
    file->addAction(quitAction);

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    settings->addAction(encryptWalletAction);
    settings->addAction(changePassphraseAction);
    settings->addSeparator();
    settings->addAction(optionsAction);

    QMenu *mining = appMenuBar->addMenu(tr("&Mining"));
    mining->addSeparator();
    mining->addAction(miningOneAction);
    mining->addAction(miningOffAction);
    //settings->addAction(miningTwoAction);
    //settings->addAction(miningThreeAction);
    //settings->addAction(miningFourAction);
    //settings->addAction(miningFiveAction);
    //settings->addAction(miningSixAction);
#if defined(Q_OS_WIN64) || defined(Q_OS_WIN32) // if windows and miner exists
    //Check pool miner exists before adding menu option
    if( access( "yam-64bit-generic.exe", F_OK ) != -1 ) {
        mining->addSeparator();
        mining->addAction(miningPoolAESONAction);
        mining->addSeparator();
        mining->addAction(balanceMMCAction);
        mining->addAction(balanceDwarfAction);
        mining->addAction(balance1GHAction);
    }
#endif

    //Check pool miner exists before adding menu option
#ifdef Q_OS_MAC
    mining->addSeparator();
    mining->addAction(miningPoolAESONAction);
    mining->addSeparator();
    mining->addAction(balanceMMCAction);
    mining->addAction(balanceDwarfAction);
    mining->addAction(balance1GHAction);
#endif

    //mining->addAction(miningPoolAESOFFAction);

    QMenu *voting = appMenuBar->addMenu(tr("&Voting"));
    voting->addAction(currentVotesAction);
    voting->addAction(currentCandidatesAction);
    voting->addAction(howToVoteAction);
    voting->addAction(currentResultsAction);


    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    help->addAction(openRPCConsoleAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void MemorycoinGUI::createToolBars()
{
    QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
    toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addAction(overviewAction);
    toolbar->addAction(sendCoinsAction);
    toolbar->addAction(receiveCoinsAction);
    toolbar->addAction(historyAction);
    toolbar->addAction(addressBookAction);
    toolbar->addAction(voteCoinsAction);
}

void MemorycoinGUI::setClientModel(ClientModel *clientModel)
{
    this->clientModel = clientModel;
    if(clientModel)
    {
        // Replace some strings and icons, when using the testnet
        if(clientModel->isTestNet())
        {
            setWindowTitle(windowTitle() + QString(" ") + tr("[testnet]"));
#ifndef Q_OS_MAC
            QApplication::setWindowIcon(QIcon(":icons/memorycoin_testnet"));
            setWindowIcon(QIcon(":icons/memorycoin_testnet"));
#else
            MacDockIconHandler::instance()->setIcon(QIcon(":icons/memorycoin_testnet"));
#endif
            if(trayIcon)
            {
                // Just attach " [testnet]" to the existing tooltip
                trayIcon->setToolTip(trayIcon->toolTip() + QString(" ") + tr("[testnet]"));
                trayIcon->setIcon(QIcon(":/icons/toolbar_testnet"));
            }

            toggleHideAction->setIcon(QIcon(":/icons/toolbar_testnet"));
            aboutAction->setIcon(QIcon(":/icons/toolbar_testnet"));
        }

        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        setNumConnections(clientModel->getNumConnections());
        connect(clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));

        setNumBlocks(clientModel->getNumBlocks(), clientModel->getNumBlocksOfPeers());
        connect(clientModel, SIGNAL(numBlocksChanged(int,int)), this, SLOT(setNumBlocks(int,int)));
	
	setMining(0, 0);
        connect(clientModel, SIGNAL(miningChanged(double,int)), this, SLOT(setMining(double,int)));
	
        // Receive and report messages from network/worker thread
        connect(clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        rpcConsole->setClientModel(clientModel);
        walletFrame->setClientModel(clientModel);
    }
}

bool MemorycoinGUI::addWallet(const QString& name, WalletModel *walletModel)
{
    return walletFrame->addWallet(name, walletModel);
}

bool MemorycoinGUI::setCurrentWallet(const QString& name)
{
    return walletFrame->setCurrentWallet(name);
}

void MemorycoinGUI::removeAllWallets()
{
    walletFrame->removeAllWallets();
}

void MemorycoinGUI::createTrayIcon()
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);

    trayIcon->setToolTip(tr("MemoryCoin client"));
    trayIcon->setIcon(QIcon(":/icons/toolbar"));
    trayIcon->show();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon);
}

void MemorycoinGUI::createTrayIconMenu()
{
    QMenu *trayIconMenu;
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-Mac OSes)
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);

    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    dockIconHandler->setMainWindow((QMainWindow *)this);
    trayIconMenu = dockIconHandler->dockMenu();
#endif

    // Configuration of the tray icon (or dock icon) icon menu
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(sendCoinsAction);
    trayIconMenu->addAction(voteCoinsAction);
    trayIconMenu->addAction(receiveCoinsAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(signMessageAction);
    trayIconMenu->addAction(verifyMessageAction);
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);
#ifndef Q_OS_MAC // This is built-in on Mac
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void MemorycoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHideAction->trigger();
    }
}
#endif

void MemorycoinGUI::saveWindowGeometry()
{
    QSettings settings;
    settings.setValue("nWindowPos", pos());
    settings.setValue("nWindowSize", size());
}

void MemorycoinGUI::restoreWindowGeometry()
{
    QSettings settings;
    QPoint pos = settings.value("nWindowPos").toPoint();
    QSize size = settings.value("nWindowSize", QSize(850, 550)).toSize();
    if (!pos.x() && !pos.y())
    {
        QRect screen = QApplication::desktop()->screenGeometry();
        pos.setX((screen.width()-size.width())/2);
        pos.setY((screen.height()-size.height())/2);
    }
    resize(size);
    move(pos);
}

void MemorycoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;
    OptionsDialog dlg;
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void MemorycoinGUI::aboutClicked()
{
    AboutDialog dlg;
    dlg.setModel(clientModel);
    dlg.exec();
}

void MemorycoinGUI::gotoOverviewPage()
{
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void MemorycoinGUI::gotoHistoryPage()
{
    if (walletFrame) walletFrame->gotoHistoryPage();
}

void MemorycoinGUI::gotoAddressBookPage()
{
    if (walletFrame) walletFrame->gotoAddressBookPage();
}

void MemorycoinGUI::gotoReceiveCoinsPage()
{
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void MemorycoinGUI::gotoSendCoinsPage(QString addr)
{
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

void MemorycoinGUI::gotoVoteCoinsPage(QString addr)
{
    if (walletFrame) walletFrame->gotoVoteCoinsPage(addr);
}

void MemorycoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void MemorycoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}

void MemorycoinGUI::setNumConnections(int count)
{
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }
    labelConnectionsIcon->setPixmap(QIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    if(count<5){
	labelConnectionsIcon->setToolTip(tr("%n active connection(s) to MemoryCoin network. Open port 1968 for more connections.", "", count));
    }else{
	labelConnectionsIcon->setToolTip(tr("%n active connection(s) to MemoryCoin network.", "", count));
    }
}

void MemorycoinGUI::setNumBlocks(int count, int nTotalBlocks)
{
    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BLOCK_SOURCE_NETWORK:
            progressBarLabel->setText(tr("Synchronizing with network..."));
            break;
        case BLOCK_SOURCE_DISK:
            progressBarLabel->setText(tr("Importing blocks from disk..."));
            break;
        case BLOCK_SOURCE_REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BLOCK_SOURCE_NONE:
            // Case: not Importing, not Reindexing and no network connection
            progressBarLabel->setText(tr("No block source available..."));
            break;
    }

    QString tooltip;

    QDateTime lastBlockDate = clientModel->getLastBlockDate();
    QDateTime currentDate = QDateTime::currentDateTime();
    int secs = lastBlockDate.secsTo(currentDate);

    if(count < nTotalBlocks)
    {
        tooltip = tr("Processed %1 of %2 (estimated) blocks of transaction history.").arg(count).arg(nTotalBlocks);
    }
    else
    {
        tooltip = tr("Processed %1 blocks of transaction history.").arg(count);
    }

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60 && count >= nTotalBlocks)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(QIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

        walletFrame->showOutOfSyncWarning(false);

        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
    }
    else
    {
        // Represent time from last generated block in human readable text
        QString timeBehindText;
        if(secs < 48*60*60)
        {
            timeBehindText = tr("%n hour(s)","",secs/(60*60));
        }
        else if(secs < 14*24*60*60)
        {
            timeBehindText = tr("%n day(s)","",secs/(24*60*60));
        }
        else
        {
            timeBehindText = tr("%n week(s)","",secs/(7*24*60*60));
        }

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(clientModel->getVerificationProgress() * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        labelBlocksIcon->setMovie(syncIconMovie);
        if(count != prevBlocks)
            syncIconMovie->jumpToNextFrame();
        prevBlocks = count;

        walletFrame->showOutOfSyncWarning(true);

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

void MemorycoinGUI::setMining(double hashrate, int threads)
{
    if (threads>0)
    {
        labelMiningIcon->setPixmap(QIcon(":/icons/mining_active").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
	if(hashrate>0){
		labelMiningIcon->setToolTip(tr("Mining. %2 thread(s) at %1 hashes per minute").arg(hashrate).arg(threads));
	}else{
		labelMiningIcon->setToolTip(tr("Mining. %1 thread(s). Still calculating hash rate - may take 4 minutes.").arg(threads));	
	}
    }
    else
    {
        labelMiningIcon->setPixmap(QIcon(":/icons/mining_inactive").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelMiningIcon->setToolTip(tr("Not mining MemoryCoin"));
    }
}

void MemorycoinGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("MemoryCoin"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    // Override title based on style
    QString msgType;
    switch (style) {
    case CClientUIInterface::MSG_ERROR:
        msgType = tr("Error");
        break;
    case CClientUIInterface::MSG_WARNING:
        msgType = tr("Warning");
        break;
    case CClientUIInterface::MSG_INFORMATION:
        msgType = tr("Information");
        break;
    default:
        msgType = title; // Use supplied title
    }
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        QMessageBox mBox((QMessageBox::Icon)nMBoxIcon, strTitle, message, buttons);
        int r = mBox.exec();
        if (ret != NULL)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify((Notificator::Class)nNotifyIcon, strTitle, message);
}

void MemorycoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
        }
    }
#endif
}

void MemorycoinGUI::closeEvent(QCloseEvent *event)
{
    if(clientModel)
    {
#ifndef Q_OS_MAC // Ignored on Mac
        if(!clientModel->getOptionsModel()->getMinimizeToTray() &&
           !clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            QApplication::quit();
        }
#endif
    }
    QMainWindow::closeEvent(event);
}

void MemorycoinGUI::askFee(qint64 nFeeRequired, bool *payFee)
{
    QString strMessage = tr("The transaction fee will be %1, "
        "which goes to the nodes that process your transaction and helps to support the network. "
        "Do you still want to process the transaction?").arg(MemorycoinUnits::formatWithUnit(MemorycoinUnits::BTC, nFeeRequired));
    QMessageBox::StandardButton retval = QMessageBox::question(
          this, tr("Confirm transaction fee"), strMessage,
          QMessageBox::Yes|QMessageBox::Cancel, QMessageBox::Yes);
    *payFee = (retval == QMessageBox::Yes);
}

void MemorycoinGUI::incomingTransaction(const QString& date, int unit, qint64 amount, const QString& type, const QString& address)
{
    // On new transaction, make an info balloon
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             tr("Date: %1\n"
                "Amount: %2\n"
                "Type: %3\n"
                "Address: %4\n")
                  .arg(date)
                  .arg(MemorycoinUnits::formatWithUnit(unit, amount, true))
                  .arg(type)
                  .arg(address), CClientUIInterface::MSG_INFORMATION);
}

void MemorycoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void MemorycoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        int nValidUrisFound = 0;
        QList<QUrl> uris = event->mimeData()->urls();
        foreach(const QUrl &uri, uris)
        {
            if (walletFrame->handleURI(uri.toString()))
                nValidUrisFound++;
        }

        // if valid URIs were found
        if (nValidUrisFound)
            walletFrame->gotoSendCoinsPage();
        else
            message(tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid MemoryCoin address or malformed URI parameters."),
                      CClientUIInterface::ICON_WARNING);
    }

    event->acceptProposedAction();
}

bool MemorycoinGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

void MemorycoinGUI::handleURI(QString strURI)
{
    // URI has to be valid
    if (!walletFrame->handleURI(strURI))
        message(tr("URI handling"), tr("URI can not be parsed! This can be caused by an invalid MemoryCoin address or malformed URI parameters."),
                  CClientUIInterface::ICON_WARNING);
}

void MemorycoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_open").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelEncryptionIcon->show();
        labelEncryptionIcon->setPixmap(QIcon(":/icons/lock_closed").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void MemorycoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    // activateWindow() (sometimes) helps with keyboard focus on Windows
    if (isHidden())
    {
        show();
        activateWindow();
    }
    else if (isMinimized())
    {
        showNormal();
        activateWindow();
    }
    else if (GUIUtil::isObscured(this))
    {
        raise();
        activateWindow();
    }
    else if(fToggleHidden)
        hide();
}

void MemorycoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void MemorycoinGUI::detectShutdown()
{
    if (ShutdownRequested())
        QMetaObject::invokeMethod(QCoreApplication::instance(), "quit", Qt::QueuedConnection);
}

void MemorycoinGUI::miningOff()
{
mapArgs["-genproclimit"] = "0";
GenerateMemorycoins(false, pwalletMain);
}

void MemorycoinGUI::miningOn(int processes)
{
mapArgs["-genproclimit"] = itostr(processes);
GenerateMemorycoins(processes!=0?true:false, pwalletMain);
}

void MemorycoinGUI::miningOne(){miningOn(-1);}
void MemorycoinGUI::miningTwo(){miningOn(2);}
void MemorycoinGUI::miningThree(){miningOn(4);}
void MemorycoinGUI::miningFour(){miningOn(8);}
void MemorycoinGUI::miningFive(){miningOn(16);}
void MemorycoinGUI::miningSix(){miningOn(32);}

void MemorycoinGUI::miningPoolAESON(){
	miningOff();
    LaunchPoolMiner();
}

void MemorycoinGUI::miningPoolAESOFF(){
    miningOff();
    LaunchPoolMiner();
}

void openWebsite(string url){
    QDesktopServices::openUrl(QUrl(QString::fromStdString(url), QUrl::TolerantMode));
}

void MemorycoinGUI::currentVotes(){
	openWebsite("http://mmcvotes.com/address/"+getDefaultWalletAddress());
}

void MemorycoinGUI::howToVote(){
	openWebsite("http://memorycoin.org/how-to-vote/");
}

void MemorycoinGUI::currentCandidates(){
	openWebsite("http://memorycoin.org/candidates/");
}

void MemorycoinGUI::currentResults(){
	openWebsite("http://mmcvotes.com/");
}

void MemorycoinGUI::balanceMMC(){
    openWebsite("http://mmcpool.com/en/user?wallet="+getDefaultWalletAddress());
}

void MemorycoinGUI::balanceDwarf(){
    openWebsite("http://dwarfpool.com/mmc/address?wallet="+getDefaultWalletAddress());
}

void MemorycoinGUI::balance1GH(){
    openWebsite("http://mmc.1gh.com/user?wallet="+getDefaultWalletAddress());
}

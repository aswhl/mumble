/* Copyright (C) 2005, Thorvald Natvig <thorvald@natvig.com>

   All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.
   - Redistributions in binary form must reproduce the above copyright notice,
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution.
   - Neither the name of the Mumble Developers nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <QMenuBar>
#include <QInputDialog>
#include <QMessageBox>
#include <QSettings>
#include <QSplitter>
#include <QTextCursor>
#include <QTime>
#include <QHeaderView>
#include <QListView>
#include <QTreeView>
#include <QWhatsThis>
#include "MainWindow.h"
#include "AudioInput.h"
#include "ConnectDialog.h"
#include "Player.h"
#include "Channel.h"
#include "TrayIcon.h"
#include "ACLEditor.h"
#include "BanEditor.h"
#include "Connection.h"
#include "ServerHandler.h"
#include "About.h"
#include "GlobalShortcut.h"
#include "VersionCheck.h"
#include "PlayerModel.h"
#include "AudioStats.h"
#include "Plugins.h"
#include "Log.h"
#ifdef Q_OS_WIN
#include "Overlay.h"
#endif
#include "Global.h"

MainWindow::MainWindow(QWidget *p) : QMainWindow(p) {
	Channel::add(0, tr("Root"), NULL);

	aclEdit = NULL;
	banEdit = NULL;

	createActions();
	setupGui();

	connect(g.sh, SIGNAL(connected()), this, SLOT(serverConnected()));
	connect(g.sh, SIGNAL(disconnected(QString)), this, SLOT(serverDisconnected(QString)));

	ti = new TrayIcon(this);
}

void MainWindow::createActions() {
	qaServerConnect=new QAction(tr("&Connect"), this);
	qaServerConnect->setToolTip(tr("Open the server connection dialog"));
	qaServerConnect->setWhatsThis(tr("Shows a dialog of registered servers, and also allows quick connect."));
	qaServerConnect->setObjectName("ServerConnect");
	qaServerDisconnect=new QAction(tr("&Disconnect"), this);
	qaServerDisconnect->setToolTip(tr("Disconnect from server"));
	qaServerDisconnect->setWhatsThis(tr("Disconnects you from the server."));
	qaServerDisconnect->setObjectName("ServerDisconnect");
	qaServerDisconnect->setEnabled(false);
	qaServerBanList=new QAction(tr("&Banlists"), this);
	qaServerBanList->setToolTip(tr("Edit banlists on server"));
	qaServerBanList->setWhatsThis(tr("This lets you edit the serverside IP banlists."));
	qaServerBanList->setObjectName("ServerBanList");
	qaServerBanList->setEnabled(false);

	qaPlayerKick=new QAction(tr("&Kick"), this);
	qaPlayerKick->setObjectName("PlayerKick");
	qaPlayerKick->setToolTip(tr("Kick player (with reason)"));
	qaPlayerKick->setWhatsThis(tr("Kick selected player off server. You'll be asked to specify a reason."));
	qaPlayerBan=new QAction(tr("&Ban"), this);
	qaPlayerBan->setObjectName("PlayerBan");
	qaPlayerBan->setToolTip(tr("Kick and ban player (with reason)"));
	qaPlayerBan->setWhatsThis(tr("Kick and ban selected player from server. You'll be asked to specify a reason."));
	qaPlayerMute=new QAction(tr("&Mute"), this);
	qaPlayerMute->setObjectName("PlayerMute");
	qaPlayerMute->setCheckable(true);
	qaPlayerMute->setToolTip(tr("Mute player"));
	qaPlayerMute->setWhatsThis(tr("Mute or unmute player on server. Unmuting a deafened player will also undeafen them."));
	qaPlayerDeaf=new QAction(tr("&Deafen"), this);
	qaPlayerDeaf->setObjectName("PlayerDeaf");
	qaPlayerDeaf->setCheckable(true);
	qaPlayerDeaf->setToolTip(tr("Deafen player"));
	qaPlayerDeaf->setWhatsThis(tr("Deafen or undeafen player on server. Deafening a player will also mute them."));

	qaChannelAdd=new QAction(tr("&Add"), this);
	qaChannelAdd->setObjectName("ChannelAdd");
	qaChannelAdd->setToolTip(tr("Add new channel"));
	qaChannelAdd->setWhatsThis(tr("This adds a new subchannel to the currently selected channel."));
	qaChannelRemove=new QAction(tr("&Remove"), this);
	qaChannelRemove->setObjectName("ChannelRemove");
	qaChannelRemove->setToolTip(tr("Remove channel"));
	qaChannelRemove->setWhatsThis(tr("This removes a channel and all subchannels."));
	qaChannelACL=new QAction(tr("&Edit ACL"), this);
	qaChannelACL->setObjectName("ChannelACL");
	qaChannelACL->setToolTip(tr("Edit Groups and ACL for channel"));
	qaChannelACL->setWhatsThis(tr("This opens the Group and ACL dialog for the channel, to control permissions."));
	qaChannelLink=new QAction(tr("&Link"), this);
	qaChannelLink->setObjectName("ChannelLink");
	qaChannelLink->setToolTip(tr("Link your channel to another channel"));
	qaChannelLink->setWhatsThis(tr("This links your current channel to the selected channel. If they have permission to speak in "
							"the other channel, players can now hear each other. This is a permanent link, and will last until "
							"manually unlinked or the server is restarted. Please see the shortcuts for push-to-link."));
	qaChannelUnlink=new QAction(tr("&Unlink"), this);
	qaChannelUnlink->setObjectName("ChannelUnlink");
	qaChannelUnlink->setToolTip(tr("Unlink your channel from another channel"));
	qaChannelUnlink->setWhatsThis(tr("This unlinks your current channel from the selected channel."));
	qaChannelUnlinkAll=new QAction(tr("Unlink &All"), this);
	qaChannelUnlinkAll->setObjectName("ChannelUnlinkAll");
	qaChannelUnlinkAll->setToolTip(tr("Unlinks your channel from all linked channels."));
	qaChannelUnlinkAll->setWhatsThis(tr("This unlinks your current channel (not the selected one) from all linked channels."));

	qaAudioReset=new QAction(tr("&Reset"), this);
	qaAudioReset->setObjectName("AudioReset");
	qaAudioReset->setToolTip(tr("Reset audio preprocessor"));
	qaAudioReset->setWhatsThis(tr("Resets the audio preprocessor, including noice cancellation, automatic gain and voice activity detection. "
									"If something suddenly worsens the audio environment (like dropping the microphone) and it was temporary, "
									"use this to avoid having to wait for the preprocessor to readjust."));
	qaAudioMute=new QAction(tr("&Mute Self"), this);
	qaAudioMute->setObjectName("AudioMute");
	qaAudioMute->setCheckable(true);
	qaAudioMute->setChecked(g.s.bMute);
	qaAudioMute->setToolTip(tr("Mute yourself"));
	qaAudioMute->setWhatsThis(tr("Mute or unmute yourself. When muted, you will not send any data to the server. Unmuting while deafened will also undeafen."));
	qaAudioDeaf=new QAction(tr("&Deafen Self"), this);
	qaAudioDeaf->setObjectName("AudioDeaf");
	qaAudioDeaf->setCheckable(true);
	qaAudioDeaf->setChecked(g.s.bDeaf);
	qaAudioDeaf->setToolTip(tr("Deafen yourself"));
	qaAudioDeaf->setWhatsThis(tr("Deafen or undeafen yourself. When deafened, you will not hear anything. Deafening yourself will also mute."));
	qaAudioTTS=new QAction(tr("&Text-To-Speech"), this);
	qaAudioTTS->setObjectName("AudioTextToSpeech");
	qaAudioTTS->setCheckable(true);
	qaAudioTTS->setChecked(g.qs->value("TextToSpeech", true).toBool());
	qaAudioTTS->setToolTip(tr("Toggle Text-To-Speech"));
	qaAudioTTS->setWhatsThis(tr("Enable or disable the text-to-speech engine. Only messages enabled for TTS in the Config dialog will actually be spoken."));
	qaAudioStats=new QAction(tr("S&tatistics"), this);
	qaAudioStats->setObjectName("AudioStats");
	qaAudioStats->setToolTip(tr("Display audio statistics"));
	qaAudioStats->setWhatsThis(tr("Pops up a small dialog with information about your current audio input."));
	qaAudioUnlink=new QAction(tr("&Unlink"), this);
	qaAudioUnlink->setObjectName("AudioUnlink");
	qaAudioUnlink->setToolTip(tr("Forcibly unlink plugin"));
	qaAudioUnlink->setWhatsThis(tr("This forces the current plugin to unlink, which is handy if it is reading "
									"completely wrong data."));

	qaConfigDialog=new QAction(tr("&Settings"), this);
	qaConfigDialog->setObjectName("ConfigDialog");
	qaConfigDialog->setToolTip(tr("Configure Mumble"));
	qaConfigDialog->setWhatsThis(tr("Allows you to change most settings for Mumble."));
	qaConfigShortcuts=new QAction(tr("&Shortcuts"), this);
	qaConfigShortcuts->setObjectName("ConfigShortcuts");
	qaConfigShortcuts->setToolTip(tr("Configure hotkeys"));
	qaConfigShortcuts->setWhatsThis(tr("Configure global hotkeys. These will work from anywhere on the system, including inside games."));

	qaHelpWhatsThis = new QAction(tr("&What's This?"), this);
	qaHelpWhatsThis->setObjectName("HelpWhatsThis");
	qaHelpWhatsThis->setToolTip(tr("Enter What's This? mode"));
	qaHelpWhatsThis->setWhatsThis(tr("Click this to enter \"What's This?\" mode. Your cursor will turn into a question mark. Click "
									"on any button, menu choice or area to show a description of what it is."));
	qaHelpAbout=new QAction(tr("&About"), this);
	qaHelpAbout->setObjectName("HelpAbout");
	qaHelpAbout->setToolTip(tr("Information about Mumble"));
	qaHelpAbout->setWhatsThis(tr("Shows a small dialog with information and license for Mumble."));
	qaHelpAboutSpeex=new QAction(tr("About &Speex"), this);
	qaHelpAboutSpeex->setObjectName("HelpAboutSpeex");
	qaHelpAboutSpeex->setToolTip(tr("Information about Speex"));
	qaHelpAboutSpeex->setWhatsThis(tr("Shows a small dialog with information about Speex."));
	qaHelpAboutQt=new QAction(tr("&About QT"), this);
	qaHelpAboutQt->setObjectName("HelpAboutQt");
	qaHelpAboutQt->setToolTip(tr("Information about Qt"));
	qaHelpAboutQt->setWhatsThis(tr("Shows a small dialog with information about Qt."));
	qaHelpVersionCheck=new QAction(tr("Check for &Updates"), this);
	qaHelpVersionCheck->setObjectName("HelpVersionCheck");
	qaHelpVersionCheck->setToolTip(tr("Check for new version of Mumble"));
	qaHelpVersionCheck->setWhatsThis(tr("Connects to the Mumble webpage to check if a new version is available, and notifies "
										"you with an appropriate download URL if this is the case."));
}

void MainWindow::setupGui()  {
	setWindowTitle(tr("Mumble -- %1").arg(QString(MUMBLE_RELEASE)));

	pmModel = new PlayerModel(this);

	QTreeView *view = new QTreeView(this);
	qtvPlayers = view;
	qtvPlayers->setObjectName("Players");
	qtvPlayers->setContextMenuPolicy(Qt::CustomContextMenu);
	qtvPlayers->setModel(pmModel);
	qtvPlayers->setItemDelegate(new PlayerDelegate(view));
	qtvPlayers->setDragEnabled(true);
	qtvPlayers->setAcceptDrops(true);
	qtvPlayers->setDropIndicatorShown(true);
	qtvPlayers->setIndentation(10);

	qteLog = new QTextEdit(this);
	qteLog->setReadOnly(true);
	qteLog->setToolTip(tr("Log of messages"));
	qteLog->setWhatsThis(tr("This shows all recent activity. Connecting to servers, errors and information messasges all show up here.<br />"
							"To configure exactly which messages show up here, use the <b>Settings</b> command from the menu."));

	qmServer = new QMenu(tr("&Server"), this);
	qmPlayer = new QMenu(tr("&Player"), this);
	qmChannel = new QMenu(tr("&Channel"), this);
	qmAudio = new QMenu(tr("&Audio"), this);
	qmConfig = new QMenu(tr("C&onfig"), this);
	qmHelp = new QMenu(tr("&Help"), this);

	qmServer->setObjectName("ServerMenu");
	qmPlayer->setObjectName("PlayerMenu");
	qmChannel->setObjectName("ChannelMenu");
	qmAudio->setObjectName("AudioMenu");
	qmConfig->setObjectName("ConfigMenu");
	qmHelp->setObjectName("HelpMenu");

	qmServer->addAction(qaServerConnect);
	qmServer->addAction(qaServerDisconnect);
	qmServer->addAction(qaServerBanList);

	qmPlayer->addAction(qaPlayerKick);
	qmPlayer->addAction(qaPlayerBan);
	qmPlayer->addAction(qaPlayerMute);
	qmPlayer->addAction(qaPlayerDeaf);

	qmChannel->addAction(qaChannelAdd);
	qmChannel->addAction(qaChannelRemove);
	qmChannel->addAction(qaChannelACL);
	qmChannel->addAction(qaChannelLink);
	qmChannel->addAction(qaChannelUnlink);
	qmChannel->addAction(qaChannelUnlinkAll);

	qmAudio->addAction(qaAudioMute);
	qmAudio->addAction(qaAudioDeaf);
	qmAudio->addSeparator();
	qmAudio->addAction(qaAudioReset);
	qmAudio->addAction(qaAudioUnlink);
	qmAudio->addSeparator();
	qmAudio->addAction(qaAudioTTS);
	qmAudio->addSeparator();
	qmAudio->addAction(qaAudioStats);

	qmConfig->addAction(qaConfigDialog);
	qmConfig->addAction(qaConfigShortcuts);

	qmHelp->addAction(qaHelpWhatsThis);
	qmHelp->addSeparator();
	qmHelp->addAction(qaHelpAbout);
	qmHelp->addAction(qaHelpAboutSpeex);
	qmHelp->addAction(qaHelpAboutQt);
	qmHelp->addSeparator();
	qmHelp->addAction(qaHelpVersionCheck);

	menuBar()->addMenu(qmServer);
	menuBar()->addMenu(qmPlayer);
	menuBar()->addMenu(qmChannel);
	menuBar()->addMenu(qmAudio);
	menuBar()->addMenu(qmConfig);
	menuBar()->addMenu(qmHelp);

	int idx = 1;

	gsPushTalk=new GlobalShortcut(this, idx++, "Push-to-Talk");
	gsPushTalk->setObjectName("PushToTalk");

	gsResetAudio=new GlobalShortcut(this, idx++, "Reset Audio Processor");
	gsResetAudio->setObjectName("ResetAudio");
	connect(gsResetAudio, SIGNAL(down()), qaAudioReset, SLOT(trigger()));

	gsMuteSelf=new GlobalShortcut(this, idx++, "Toggle Mute Self");
	gsMuteSelf->setObjectName("MuteSelf");
	connect(gsMuteSelf, SIGNAL(down()), qaAudioMute, SLOT(trigger()));

	gsDeafSelf=new GlobalShortcut(this, idx++, "Toggle Deafen Self");
	gsDeafSelf->setObjectName("DeafSelf");
	connect(gsDeafSelf, SIGNAL(down()), qaAudioDeaf, SLOT(trigger()));

	gsUnlink=new GlobalShortcut(this, idx++, "Unlink Plugin");
	gsUnlink->setObjectName("UnlinkPlugin");
	connect(gsUnlink, SIGNAL(down()), qaAudioUnlink, SLOT(trigger()));

	gsCenterPos=new GlobalShortcut(this, idx++, "Force Center Position");
	gsCenterPos->setObjectName("CenterPos");

	GlobalShortcut *gs;

	gs = new GlobalShortcut(this, idx++, "Chan Parent");
	gs->setData(0);
	connect(gs, SIGNAL(triggered(bool)), this, SLOT(pushLink(bool)));

	for(int i = 1; i< 10;i++) {
		gs = new GlobalShortcut(this, idx++, QString("Chan Sub#%1").arg(i));
		gs->setData(i);
		connect(gs, SIGNAL(triggered(bool)), this, SLOT(pushLink(bool)));
	}

	gs = new GlobalShortcut(this, idx++, "Chan All Subs");
	gs->setData(10);
	connect(gs, SIGNAL(triggered(bool)), this, SLOT(pushLink(bool)));

	gsPushMute=new GlobalShortcut(this, idx++, "Push-to-Mute");
	gsPushMute->setObjectName("PushToMute");

	gsMetaChannel=new GlobalShortcut(this, idx++, "Join Channel");
	gsMetaChannel->setObjectName("MetaChannel");

#ifdef Q_OS_WIN
	gsToggleOverlay=new GlobalShortcut(this, idx++, "Toggle Overlay");
	gsToggleOverlay->setObjectName("ToggleOverlay");
	connect(gsToggleOverlay, SIGNAL(down()), g.o, SLOT(toggleShow()));
#endif

	qsSplit = new QSplitter(Qt::Horizontal, this);
	qsSplit->addWidget(qteLog);
	qsSplit->addWidget(qtvPlayers);

	setCentralWidget(qsSplit);

	restoreState(g.qs->value("mw").toByteArray());
	qsSplit->restoreState(g.qs->value("mwSplitter").toByteArray());

	QPoint ps = g.qs->value("mwPos").toPoint();
	if (! ps.isNull() && (ps.x() >= 0) && (ps.y() >= 0)) {
		move(ps);
		QSize sz = g.qs->value("mwSize").toSize();
		if (sz.isValid())
			resize(sz);
	}

    QMetaObject::connectSlotsByName(this);
}

void MainWindow::closeEvent(QCloseEvent *e) {
	g.sId = 0;
	g.qs->setValue("mwPos", pos());
	g.qs->setValue("mwSize", size());
	g.qs->setValue("mw", saveState());
	g.qs->setValue("mwSplitter", qsSplit->saveState());
	QMainWindow::closeEvent(e);
}

void MainWindow::appendLog(QString entry)
{
	qteLog->append(entry);
	QTextCursor p=qteLog->textCursor();
	p.movePosition(QTextCursor::End);
	qteLog->setTextCursor(p);
	qteLog->ensureCursorVisible();
}

void MainWindow::on_Players_customContextMenuRequested(const QPoint &mpos) {
	QModelIndex idx = qtvPlayers->indexAt(mpos);
	if (! idx.isValid())
		idx = qtvPlayers->currentIndex();
	Player *p = pmModel->getPlayer(idx);

	if (p) {
		qmPlayer->popup(qtvPlayers->mapToGlobal(mpos), qaPlayerMute);
	} else {
		qmChannel->popup(qtvPlayers->mapToGlobal(mpos), qaChannelACL);
	}
}

void MainWindow::on_Players_doubleClicked(const QModelIndex &idx) {
	Channel *c = pmModel->getChannel(idx);
	if (!c)
		return;
	MessagePlayerMove mpm;
	mpm.sVictim = g.sId;
	mpm.iChannelId = c->iId;
	g.sh->sendMessage(&mpm);
}

void MainWindow::on_ServerConnect_triggered()
{
	ConnectDialog *cd = new ConnectDialog(this);
	int res = cd->exec();

	if (res == QDialog::Accepted) {
		qaServerConnect->setEnabled(false);
		qaServerDisconnect->setEnabled(true);
		g.sh->setConnectionInfo(cd->qsServer, cd->iPort, cd->bUdp, cd->qsUsername, cd->qsPassword);
		g.sh->start(QThread::TimeCriticalPriority);
	}
	delete cd;
}

void MainWindow::on_ServerDisconnect_triggered()
{
	g.sh->disconnect();
}

void MainWindow::on_ServerBanList_triggered()
{
	MessageServerBanList msbl;
	msbl.bQuery = true;
	g.sh->sendMessage(&msbl);

	if (banEdit) {
		banEdit->reject();
		delete banEdit;
		banEdit = NULL;
	}
}

void MainWindow::on_PlayerMenu_aboutToShow()
{
	Player *p = pmModel->getPlayer(qtvPlayers->currentIndex());
	if (! p) {
		qaPlayerKick->setEnabled(false);
		qaPlayerBan->setEnabled(false);
		qaPlayerMute->setEnabled(false);
		qaPlayerDeaf->setEnabled(false);
	} else {
		qaPlayerKick->setEnabled(true);
		qaPlayerBan->setEnabled(true);
		qaPlayerMute->setEnabled(true);
		qaPlayerDeaf->setEnabled(true);
		qaPlayerMute->setChecked(p->bMute);
		qaPlayerDeaf->setChecked(p->bDeaf);
	}
}

void MainWindow::on_PlayerMute_triggered()
{
	Player *p = pmModel->getPlayer(qtvPlayers->currentIndex());
	if (!p)
		return;

	MessagePlayerMute mpmMsg;
	mpmMsg.sVictim = p->sId;
	mpmMsg.bMute = ! p->bMute;
	g.sh->sendMessage(&mpmMsg);
}

void MainWindow::on_PlayerDeaf_triggered()
{
	Player *p = pmModel->getPlayer(qtvPlayers->currentIndex());
	if (!p)
		return;

	MessagePlayerDeaf mpdMsg;
	mpdMsg.sVictim = p->sId;
	mpdMsg.bDeaf = ! p->bDeaf;
	g.sh->sendMessage(&mpdMsg);
}

void MainWindow::on_PlayerKick_triggered()
{
	Player *p = pmModel->getPlayer(qtvPlayers->currentIndex());
	if (!p)
		return;

	bool ok;
	QString reason = QInputDialog::getText(this, tr("Kicking player %1").arg(p->qsName), tr("Enter reason"), QLineEdit::Normal, "", &ok);
	if (ok) {
		MessagePlayerKick mpkMsg;
		mpkMsg.sVictim=p->sId;
		mpkMsg.qsReason = reason;
		g.sh->sendMessage(&mpkMsg);
	}
}

void MainWindow::on_PlayerBan_triggered()
{
	Player *p = pmModel->getPlayer(qtvPlayers->currentIndex());
	if (!p)
		return;

	bool ok;
	QString reason = QInputDialog::getText(this, tr("Banning player %1").arg(p->qsName), tr("Enter reason"), QLineEdit::Normal, "", &ok);
	if (ok) {
		MessagePlayerBan mpbMsg;
		mpbMsg.sVictim=p->sId;
		mpbMsg.qsReason = reason;
		g.sh->sendMessage(&mpbMsg);
	}
}

void MainWindow::on_ChannelMenu_aboutToShow()
{
	QModelIndex idx = qtvPlayers->currentIndex();

	bool add, remove, acl, link, unlink, unlinkall;

	add = remove = acl = link = unlink = unlinkall = false;

	if (g.sId != 0) {
		add = true;
		acl = true;

		Channel *c = pmModel->getChannel(idx);
		Channel *home = Player::get(g.sId)->cChannel;

		if (c && c->iId != 0)
			remove = true;
		if (! c)
			c = Channel::get(0);

		unlinkall = (home->qhLinks.count() > 0);

		if (home != c) {
			if (c->allLinks().contains(home))
				unlink = true;
			else
				link = true;
		}
	}

	qaChannelAdd->setEnabled(add);
	qaChannelRemove->setEnabled(remove);
	qaChannelACL->setEnabled(acl);
	qaChannelLink->setEnabled(link);
	qaChannelUnlink->setEnabled(unlink);
	qaChannelUnlinkAll->setEnabled(unlinkall);
}

void MainWindow::on_ChannelAdd_triggered()
{
	bool ok;
	Channel *c = pmModel->getChannel(qtvPlayers->currentIndex());
	int iParent = c ? c->iId : 0;
	QString name = QInputDialog::getText(this, tr("Mumble"), tr("Channel Name"), QLineEdit::Normal, "", &ok);
	if (ok) {
		MessageChannelAdd mca;
		mca.qsName = name;
		mca.iParent = iParent;
		g.sh->sendMessage(&mca);
	}
}

void MainWindow::on_ChannelRemove_triggered()
{
	int ret;
	Channel *c = pmModel->getChannel(qtvPlayers->currentIndex());
	if (! c)
		return;

	ret=QMessageBox::question(this, tr("Mumble"), tr("Are you sure you want to delete %1 and all it's subchannels?").arg(c->qsName), QMessageBox::Yes, QMessageBox::No);

	if (ret == QMessageBox::Yes ) {
		MessageChannelRemove mcr;
		mcr.iId = c->iId;
		g.sh->sendMessage(&mcr);
	}
}

void MainWindow::on_ChannelACL_triggered()
{
	Channel *c = pmModel->getChannel(qtvPlayers->currentIndex());
	int id = c ? c->iId : 0;

	MessageEditACL mea;
	mea.iId = id;
	mea.bQuery = true;
	g.sh->sendMessage(&mea);

	if (aclEdit) {
		aclEdit->reject();
		delete aclEdit;
		aclEdit = NULL;
	}
}

void MainWindow::on_ChannelLink_triggered()
{
	Channel *c = Player::get(g.sId)->cChannel;
	Channel *l = pmModel->getChannel(qtvPlayers->currentIndex());
	if (! l)
		l = Channel::get(0);

	MessageChannelLink mcl;
	mcl.iId = c->iId;
	mcl.qlTargets << l->iId;
	mcl.ltType = MessageChannelLink::Link;
	g.sh->sendMessage(&mcl);
}

void MainWindow::on_ChannelUnlink_triggered()
{
	Channel *c = Player::get(g.sId)->cChannel;
	Channel *l = pmModel->getChannel(qtvPlayers->currentIndex());
	if (! l)
		l = Channel::get(0);

	MessageChannelLink mcl;
	mcl.iId = c->iId;
	mcl.qlTargets << l->iId;
	mcl.ltType = MessageChannelLink::Unlink;
	g.sh->sendMessage(&mcl);
}

void MainWindow::on_ChannelUnlinkAll_triggered()
{
	Channel *c = Player::get(g.sId)->cChannel;

	MessageChannelLink mcl;
	mcl.iId = c->iId;
	mcl.ltType = MessageChannelLink::UnlinkAll;
	g.sh->sendMessage(&mcl);
}

void MainWindow::on_AudioReset_triggered()
{
	AudioInputPtr ai = g.ai;
	if (ai)
		ai->bResetProcessor = true;
}

void MainWindow::on_AudioMute_triggered()
{
	g.s.bMute = qaAudioMute->isChecked();
	if (! g.s.bMute && g.s.bDeaf) {
		g.s.bDeaf = false;
		qaAudioDeaf->setChecked(false);
		g.l->log(Log::SelfMute, tr("Un-muted and undeafened."));
	} else if (! g.s.bMute) {
		g.l->log(Log::SelfMute, tr("Unmuted."));
	} else {
		g.l->log(Log::SelfMute, tr("Muted."));
	}

	MessagePlayerSelfMuteDeaf mpsmd;
	mpsmd.bMute = g.s.bMute;
	mpsmd.bDeaf = g.s.bDeaf;
	g.sh->sendMessage(&mpsmd);
}

void MainWindow::on_AudioDeaf_triggered()
{
	g.s.bDeaf = qaAudioDeaf->isChecked();
	if (g.s.bDeaf && ! g.s.bMute) {
		g.s.bMute = true;
		qaAudioMute->setChecked(true);
		g.l->log(Log::SelfMute, tr("Muted and deafened."));
	} else if (g.s.bDeaf) {
		g.l->log(Log::SelfMute, tr("Deafened."));
	} else {
		g.l->log(Log::SelfMute, tr("Undeafened."));
	}

	MessagePlayerSelfMuteDeaf mpsmd;
	mpsmd.bMute = g.s.bMute;
	mpsmd.bDeaf = g.s.bDeaf;
	g.sh->sendMessage(&mpsmd);
}

void MainWindow::on_AudioTextToSpeech_triggered()
{
	g.s.bTTS = qaAudioTTS->isChecked();
}

void MainWindow::on_AudioStats_triggered()
{
	AudioStats *as=new AudioStats(this);
	as->show();
}

void MainWindow::on_AudioUnlink_triggered()
{
	g.p->bUnlink = true;
}

void MainWindow::on_ConfigDialog_triggered()
{
	ConfigDialog dlg;
	dlg.exec();
}

void MainWindow::on_ConfigShortcuts_triggered()
{
	GlobalShortcut::configure();
}

void MainWindow::on_HelpWhatsThis_triggered()
{
	QWhatsThis::enterWhatsThisMode();
}

void MainWindow::on_HelpAbout_triggered()
{
	AboutDialog adAbout(this);
	adAbout.exec();
}

void MainWindow::on_HelpAboutSpeex_triggered()
{
	AboutSpeexDialog adAbout(this);
	adAbout.exec();
}

void MainWindow::on_HelpAboutQt_triggered()
{
	QMessageBox::aboutQt(this, tr("About Qt"));
}

void MainWindow::on_HelpVersionCheck_triggered()
{
	new VersionCheck(this);
}

void MainWindow::on_PushToTalk_triggered(bool down)
{
	if (down)
		g.iPushToTalk++;
	else
		g.iPushToTalk--;
}

void MainWindow::on_PushToMute_triggered(bool down)
{
	g.bPushToMute = down;
}

void MainWindow::on_CenterPos_triggered(bool down)
{
	g.bCenterPosition = down;

	if (down)
		g.iPushToTalk++;
	else
		g.iPushToTalk--;
}

void MainWindow::pushLink(bool down)
{
	if (down)
		g.iPushToTalk++;
	else
		g.iPushToTalk--;

	if (g.sId == 0)
		return;

	GlobalShortcut *gs = qobject_cast<GlobalShortcut *>(sender());
	int idx = gs->data().toInt();
	Channel *home = Player::get(g.sId)->cChannel;

	Channel *target = NULL;
	switch(idx) {
		case 0:
			target = home->cParent;
			break;
		case 10:
			break;
		default:
			target = pmModel->getSubChannel(home, idx-1);
			break;
	}


	if (gsMetaChannel->active()) {
		if (! target || ! down)
			return;

		MessagePlayerMove mpm;
		mpm.sVictim = g.sId;
		mpm.iChannelId = target->iId;
		g.sh->sendMessage(&mpm);
		g.l->log(Log::Information, tr("Joining %1.").arg(target->qsName));
	} else {
		MessageChannelLink mcl;
		mcl.iId = home->iId;
		if (down)
			mcl.ltType = MessageChannelLink::PushLink;
		else
			mcl.ltType = MessageChannelLink::PushUnlink;
		if (idx == 10) {
			foreach(Channel *l, home->qlChannels)
				mcl.qlTargets << l->iId;
		} else if (target) {
				mcl.qlTargets << target->iId;
		}
		if (mcl.qlTargets.count() == 0)
			return;
		g.sh->sendMessage(&mcl);
	}
}

void MainWindow::serverConnected()
{
	g.sId = 0;
	g.l->clearIgnore();
	g.l->setIgnore(Log::PlayerJoin);
	g.l->setIgnore(Log::OtherSelfMute);
	g.l->log(Log::ServerConnected, tr("Connected to server."));
	qaServerDisconnect->setEnabled(true);
	qaServerBanList->setEnabled(true);

	if (g.s.bMute || g.s.bDeaf) {
		MessagePlayerSelfMuteDeaf mpsmd;
		mpsmd.bMute = g.s.bMute;
		mpsmd.bDeaf = g.s.bDeaf;
		g.sh->sendMessage(&mpsmd);
	}
}

void MainWindow::serverDisconnected(QString reason)
{
	g.sId = 0;
	qaServerConnect->setEnabled(true);
	qaServerDisconnect->setEnabled(false);
	qaServerBanList->setEnabled(false);

	if (aclEdit) {
		aclEdit->reject();
		delete aclEdit;
		aclEdit = NULL;
	}

	if (banEdit) {
		banEdit->reject();
		delete banEdit;
		banEdit = NULL;
	}

	pmModel->removeAll();

	if (! reason.isEmpty()) {
  	  g.l->log(Log::ServerDisconnected, tr("Server connection failed: %1.").arg(reason));
    } else {
	  g.l->log(Log::ServerDisconnected, tr("Disconnected from server."));
	}
}

void MainWindow::customEvent(QEvent *evt) {
	if (evt->type() != SERVERSEND_EVENT)
		return;

	ServerHandlerMessageEvent *shme=static_cast<ServerHandlerMessageEvent *>(evt);

	Message *mMsg = Message::networkToMessage(shme->qbaMsg);
	if (mMsg) {
		mMsg->process(NULL);
		delete mMsg;
	}
}

void MessageServerJoin::process(Connection *) {
	Player *p = g.mw->pmModel->addPlayer(sPlayerId, qsPlayerName);
	p->iId = iId;
	g.l->log(Log::PlayerJoin, MainWindow::tr("Joined server: %1.").arg(p->qsName));
}

#define MSG_INIT \
	Player *pSrc=Player::get(sPlayerId); \
	Q_UNUSED(pSrc);

#define VICTIM_INIT \
	Player *pDst=Player::get(sVictim); \
	 if (! pDst) \
 		qFatal("MainWindow: Message for nonexistant victim %d.", sVictim);

void MessageServerLeave::process(Connection *) {
	MSG_INIT;

	g.l->log(Log::PlayerLeave, MainWindow::tr("Left server: %1.").arg(pSrc->qsName));
	g.mw->pmModel->removePlayer(pSrc);
}

void MessageServerBanList::process(Connection *) {
	MSG_INIT;

	if (g.mw->banEdit) {
		g.mw->banEdit->reject();
		delete g.mw->banEdit;
		g.mw->banEdit = NULL;
	}
	g.mw->banEdit = new BanEditor(this, g.mw);
	g.mw->banEdit->show();

}

void MessageSpeex::process(Connection *) {
}

void MessageMultiSpeex::process(Connection *) {
}

void MessagePlayerSelfMuteDeaf::process(Connection *) {
	MSG_INIT;

	pSrc->setSelfMuteDeaf(bMute, bDeaf);

	if (sPlayerId == g.sId || ! g.sId)
		return;
	if (pSrc->cChannel != Player::get(g.sId)->cChannel)
		return;

	QString name = pSrc->qsName;
	if (bMute && bDeaf)
		g.l->log(Log::OtherSelfMute, MainWindow::tr("%1 is now muted and deafened.").arg(name));
	else if (bMute)
		g.l->log(Log::OtherSelfMute, MainWindow::tr("%1 is now muted.").arg(name));
	else
		g.l->log(Log::OtherSelfMute, MainWindow::tr("%1 is now unmuted.").arg(name));
}

void MessagePlayerMute::process(Connection *) {
	MSG_INIT;
	VICTIM_INIT;

	pDst->setMute(bMute);

	if (!g.sId || pDst->cChannel != Player::get(g.sId)->cChannel)
		return;

	QString vic = pDst->qsName;
	QString admin = pSrc ? pSrc->qsName : MainWindow::tr("server");

	if (sVictim == g.sId)
		g.l->log(Log::YouMuted, bMute ? MainWindow::tr("You were muted by %1.").arg(admin) : MainWindow::tr("You were unmuted by %1.").arg(admin));
	else
		g.l->log((sPlayerId == g.sId) ? Log::YouMutedOther : Log::OtherMutedOther, bMute ? MainWindow::tr("%1 muted by %2.").arg(vic).arg(admin) : MainWindow::tr("%1 unmuted by %2.").arg(vic).arg(admin), QString());
}

void MessagePlayerDeaf::process(Connection *) {
	MSG_INIT;
	VICTIM_INIT;

	pDst->setDeaf(bDeaf);

	if (!g.sId || pDst->cChannel != Player::get(g.sId)->cChannel)
		return;

	QString vic = pDst->qsName;
	QString admin = pSrc ? pSrc->qsName : MainWindow::tr("server");

	if (sVictim == g.sId)
		g.l->log(Log::YouMuted, bDeaf ? MainWindow::tr("You were deafened by %1.").arg(admin) : MainWindow::tr("You were undeafened by %1.").arg(admin));
	else
		g.l->log((sPlayerId == g.sId) ? Log::YouMutedOther : Log::OtherMutedOther, bDeaf ? MainWindow::tr("%1 defened by %2.").arg(vic).arg(admin) : MainWindow::tr("%1 undeafened by %2.").arg(vic).arg(admin), QString());
}

void MessagePlayerKick::process(Connection *) {
	MSG_INIT;
	VICTIM_INIT;
	if (sVictim == g.sId) {
		g.l->log(Log::YouKicked, MainWindow::tr("You were kicked from the server by %1: %2.").arg(pSrc->qsName).arg(qsReason));
		g.l->setIgnore(Log::ServerDisconnected, 1);
	} else {
		g.l->setIgnore(Log::PlayerLeave, 1);
		g.l->log((sPlayerId == g.sId) ? Log::YouKicked : Log::PlayerKicked, MainWindow::tr("%3 was kicked from the server by %1: %2.").arg(pSrc->qsName).arg(qsReason).arg(pDst->qsName));
	}
}

void MessagePlayerBan::process(Connection *) {
	MSG_INIT;
	VICTIM_INIT;
	if (sVictim == g.sId) {
		g.l->log(Log::YouKicked, MainWindow::tr("You were kicked and banned from the server by %1: %2.").arg(pSrc->qsName).arg(qsReason));
		g.l->setIgnore(Log::ServerDisconnected, 1);
	} else {
		g.l->setIgnore(Log::PlayerLeave, 1);
		g.l->log((sPlayerId == g.sId) ? Log::YouKicked : Log::PlayerKicked, MainWindow::tr("%3 was kicked and banned from the server by %1: %2.").arg(pSrc->qsName).arg(qsReason).arg(pDst->qsName));
	}
}

void MessagePlayerMove::process(Connection *) {
	MSG_INIT;
	VICTIM_INIT;

	bool log = true;
	if ((sVictim == g.sId) && (sPlayerId == sVictim))
		log = false;
	if (g.sId == 0)
		log = false;

	QString pname = pDst->qsName;
	QString admin = pSrc ? pSrc->qsName : "server";

	if (log && (pDst->cChannel == Player::get(g.sId)->cChannel)) {
		if (pDst == pSrc || (!pSrc))
			g.l->log(Log::ChannelJoin, MainWindow::tr("%1 left channel.").arg(pname));
		else
			g.l->log(Log::ChannelJoin, MainWindow::tr("%1 moved out by %2").arg(pname).arg(admin));
	}

	g.mw->pmModel->movePlayer(pDst, iChannelId);

	if (log && (pDst->cChannel == Player::get(g.sId)->cChannel)) {
		if (pDst == pSrc || (!pSrc))
			g.l->log(Log::ChannelLeave, MainWindow::tr("%1 entered channel.").arg(pname));
		else
			g.l->log(Log::ChannelLeave, MainWindow::tr("%1 moved in by %2.").arg(pname).arg(admin));
	}
}

void MessagePlayerRename::process(Connection *) {
	MSG_INIT;
	g.mw->pmModel->renamePlayer(pSrc, qsName);
}

void MessageChannelAdd::process(Connection *) {
	Channel *p = Channel::get(iParent);
	if (p)
		g.mw->pmModel->addChannel(iId, p, qsName);
}

void MessageChannelRemove::process(Connection *) {
	Channel *c = Channel::get(iId);
	if (c)
		g.mw->pmModel->removeChannel(c);
}

void MessageChannelMove::process(Connection *) {
	g.mw->pmModel->moveChannel(Channel::get(iId), iParent);
}

void MessageChannelLink::process(Connection *) {
	Channel *c = Channel::get(iId);
	QList<Channel *> qlChans;
	foreach(int id, qlTargets) {
		Channel *l = Channel::get(id);
		qlChans << l;
	}

	switch (ltType) {
		case Link:
			g.mw->pmModel->linkChannels(c, qlChans);
			break;
		case Unlink:
			g.mw->pmModel->unlinkChannels(c, qlChans);
			break;
		case UnlinkAll:
			g.mw->pmModel->unlinkAll(c);
			break;
		default:
			qFatal("Unknown link message");
	}
}

void MessageServerAuthenticate::process(Connection *) {
}

void MessageServerReject::process(Connection *) {
	g.l->log(Log::ServerDisconnected, MainWindow::tr("Server connection rejected: %1.").arg(qsReason));
	g.l->setIgnore(Log::ServerDisconnected, 1);
}

void MessagePermissionDenied::process(Connection *) {
	g.l->log(Log::PermissionDenied, MainWindow::tr("Denied: %1.").arg(qsReason));
}

void MessageServerSync::process(Connection *) {
	MSG_INIT;
	g.iMaxBandwidth = iMaxBandwidth;
	g.sId = sPlayerId;
	g.l->clearIgnore();
	g.l->log(Log::Information, qsWelcomeText, QString(""));
	g.mw->pmModel->ensureSelfVisible();
}

void MessageEditACL::process(Connection *) {
	if (g.mw->aclEdit) {
		g.mw->aclEdit->reject();
		delete g.mw->aclEdit;
		g.mw->aclEdit = NULL;
	}
	g.mw->aclEdit = new ACLEditor(this, g.mw);
	g.mw->aclEdit->show();
}

void MessageQueryUsers::process(Connection *) {
	if (g.mw->aclEdit)
		g.mw->aclEdit->returnQuery(this);
}

void MessagePing::process(Connection *) {
}

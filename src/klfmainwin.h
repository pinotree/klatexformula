/***************************************************************************
 *   file klfmainwin.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2007 by Philippe Faist
 *   philippe.faist at bluewin.ch
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
/* $Id$ */

#ifndef KLFMAINWIN_H
#define KLFMAINWIN_H

#include <stdio.h>

#include <QObject>
#include <QList>
#include <QFont>
#include <QCheckBox>
#include <QMenu>
#include <QTextEdit>
#include <QWidget>
#include <QMimeData>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QDialog>

#include <klfbackend.h>

#include <klflib.h>
#include <klfconfig.h>
#include <klflatexsymbols.h>
//#include <klflatexsyntaxhighlighter.h>



class KLFLibBrowser;
//class KLFLatexSymbols;
//class KLFLatexSymbol;
class KLFStyleManager;
class KLFSettings;
class KLFLatexSyntaxHighlighter;



namespace Ui {
  class KLFProgErr;
  class KLFMainWin;
}

class KLF_EXPORT KLFProgErr : public QDialog
{
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  static void showError(QWidget *parent, QString text);

private:
  Ui::KLFProgErr *u;
};



/**
 * A helper that runs in a different thread that generates previews in real-time as user types text,
 * without blocking the GUI.
 */
class KLF_EXPORT KLFPreviewBuilderThread : public QThread
{
  Q_OBJECT

public:
  KLFPreviewBuilderThread(QObject *parent, KLFBackend::klfInput input, KLFBackend::klfSettings settings,
			  int labelwidth, int labelheight);
  virtual ~KLFPreviewBuilderThread();
  void run();

signals:
  void previewAvailable(const QImage& preview, bool latexerror);

public slots:
  bool inputChanged(const KLFBackend::klfInput& input);
  void settingsChanged(const KLFBackend::klfSettings& settings, int labelwidth, int labelheight);

protected:
  KLFBackend::klfInput _input;
  KLFBackend::klfSettings _settings;
  int _lwidth, _lheight;

  QMutex _mutex;
  QWaitCondition _condnewinfoavail;

  bool _hasnewinfo;
  bool _abort;
};


class KLFAboutDialog;
class KLFWhatsNewDialog;
class KLFMainWinPopup;

/**
 * KLatexFormula Main Window
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLF_EXPORT KLFMainWin : public QWidget
{
  Q_OBJECT
  Q_PROPERTY(QString widgetStyle READ widgetStyle WRITE setWidgetStyle)

public:
  KLFMainWin();
  virtual ~KLFMainWin();

  bool eventFilter(QObject *obj, QEvent *event);

  KLFStyle currentStyle() const;

  KLFBackend::klfSettings backendSettings() const { return _settings; }

  virtual QFont txtLatexFont() const;
  virtual QFont txtPreambleFont() const;

  KLFBackend::klfSettings currentSettings() const { return _settings; }

  void applySettings(const KLFBackend::klfSettings& s);

  KLFBackend::klfOutput currentKLFBackendOutput() const { return _output; }

  enum altersetting_which { altersetting_LBorderOffset = 100,
			    altersetting_TBorderOffset,
			    altersetting_RBorderOffset,
			    altersetting_BBorderOffset,
			    altersetting_TempDir,
			    altersetting_Latex,
			    altersetting_Dvips,
			    altersetting_Gs,
			    altersetting_Epstopdf,
			    altersetting_OutlineFonts //!< bool given as an int value
  };
  void alterSetting(altersetting_which, int ivalue);
  void alterSetting(altersetting_which, QString svalue);

  QMimeData * resultToMimeData();
  
  KLFLibBrowser * libBrowserWidget() { return mLibBrowser; }
  KLFLatexSymbols * latexSymbolsWidget() { return mLatexSymbols; }
  KLFStyleManager * styleManagerWidget() { return mStyleManager; }
  KLFSettings * settingsDialog() { return mSettingsDialog; }
  QMenu * styleMenu() { return mStyleMenu; }
  KLFLatexSyntaxHighlighter * syntaxHighlighter() { return mHighlighter; }
  KLFLatexSyntaxHighlighter * preambleSyntaxHighlighter() { return mPreambleHighlighter; }

  KLFConfig * klfConfig() { return & klfconfig; }

  QHash<QWidget*,bool> currentWindowShownStatus(bool mainWindowToo = false);
  QHash<QWidget*,bool> prepareAllWindowShownStatus(bool visibleStatus, bool mainWindowToo = false);

  QString widgetStyle() const { return _widgetstyle; }

  void registerHelpLinkAction(const QString& path, QObject *object, const char * member, bool wantUrlParam);

signals:

  void evaluateFinished(const KLFBackend::klfOutput& output);

  // dialogs (e.g. stylemanager) should connect to this in case styles change unexpectedly
  void stylesChanged();

  void applicationLocaleChanged(const QString& newLocale);

public slots:

  void slotEvaluate();
  void slotClear();
  void slotLibrary(bool showlib);
  void slotLibraryButtonRefreshState(bool on);
  void slotSymbols(bool showsymbs);
  void slotSymbolsButtonRefreshState(bool on);
  void slotExpandOrShrink();
  void slotSetLatex(const QString& latex);
  void slotSetMathMode(const QString& mathmode);
  void slotSetPreamble(const QString& preamble);
  /** If \c line is already in the preamble, then does nothing. Otherwise appends
   * \c line to the preamble text. */
  void slotEnsurePreambleCmd(const QString& line);
  void slotSetDPI(int DPI);
  void slotSetFgColor(const QColor& fgcolor);
  void slotSetFgColor(const QString& fgcolor);
  void slotSetBgColor(const QColor& bgcolor);
  void slotSetBgColor(const QString& bgcolor);

  // will actually save only if output non empty.
  void slotEvaluateAndSave(const QString& output, const QString& format);

  bool importCmdlKLFFiles(const QStringList& files, bool showLibrary = true);
  bool importCmdlKLFFile(const QString& file, bool showLibrary = false);

  void setApplicationLocale(const QString& locale);

  void retranslateUi(bool alsoBaseUi = true);

  bool loadNamedStyle(const QString& sty);

  void slotDrag();
  void slotCopy();
  void slotSave(const QString& suggestedFname = QString::null);

  void slotActivateEditor();

  void slotShowBigPreview();

  void slotPresetDPISender();
  void slotLoadStyle(int stylenum);
  void slotLoadStyle(const KLFStyle& style);
  void slotSaveStyle();
  void slotStyleManager();
  void slotSettings();


  void refreshWindowSizes();

  void refreshStylePopupMenus();
  void loadStyles();
  void loadLibrary(); // load library stuff
  void loadLibrarySavedState();
  void saveStyles();
  void restoreFromLibrary(const KLFLibEntry& entry, uint restoreflags);
  void insertSymbol(const KLFLatexSymbol& symbol);
  void saveSettings();
  void saveLibraryState();
  void loadSettings();

  void addWhatsNewText(const QString& htmlSnipplet);

  void showAbout();
  void showWhatsNew();
  void showSettingsHelpLinkAction(const QUrl& link);
  void helpLinkAction(const QUrl& link);

  void setWidgetStyle(const QString& qtstyle);

  void setTxtLatexFont(const QFont& f);
  void setTxtPreambleFont(const QFont& f);

  void showRealTimePreview(const QImage& preview, bool latexerror);

  void updatePreviewBuilderThreadInput();

  void displayError(const QString& errormsg);

  void setWindowShownStatus(const QHash<QWidget*,bool>& windowshownflags);

  void setQuitOnClose(bool quitOnClose);

  void quit();

private slots:
  // private : only as slot to an action containing the style # as user data
  void slotLoadStyleAct();

  void slotOpenHistoryLibraryResource();

  void slotNewSymbolTyped(const QString& symbol);
  void slotPopupClose();
  void slotPopupAction(const QUrl& helpLinkUrl);
  void slotPopupAcceptAll();

  void slotEditorContextMenu(const QPoint& pos);

protected:
  Ui::KLFMainWin *u;

  KLFLibBrowser *mLibBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;
  KLFSettings *mSettingsDialog;
  KLFAboutDialog *mAboutDialog;
  KLFWhatsNewDialog *mWhatsNewDialog;

  KLFMainWinPopup *mPopup;

  /** \internal */
  struct HelpLinkAction {
    HelpLinkAction(const QString& p, QObject *obj, const char *func, bool param)
      : path(p), reciever(obj), memberFunc(func), wantParam(param) { }
    QString path;
    QObject *reciever;
    QByteArray memberFunc;
    bool wantParam;
  };
  QList<HelpLinkAction> mHelpLinkActions;

  KLFLibResourceEngine *mHistoryLibResource;

  QMenu *mStyleMenu;

  KLFLatexSyntaxHighlighter *mHighlighter;
  KLFLatexSyntaxHighlighter *mPreambleHighlighter;

  bool _loadedlibrary;
  bool _firstshow;

  KLFBackend::klfSettings _settings; // settings we pass to KLFBackend
  bool _settings_altered;

  KLFBackend::klfOutput _output; // output from KLFBackend
  KLFBackend::klfInput _lastrun_input; // input that generated _output

  /** If TRUE, then the output contained in _output is up-to-date, meaning that we favor displaying
   * _output.result instead of the image given by mPreviewBuilderThread. */
  bool _evaloutput_uptodate;
  /** The Thread that will create real-time previews of formulas. */
  KLFPreviewBuilderThread *mPreviewBuilderThread;

  KLFStyleList _styles;

  QSize _shrinkedsize;
  QSize _expandedsize;

  KLFBackend::klfInput collectInput();

  bool event(QEvent *e);
  void childEvent(QChildEvent *e);
  void closeEvent(QCloseEvent *e);
  void hideEvent(QHideEvent *e);
  void showEvent(QShowEvent *e);

  bool _ignore_close_event;

  QList<QWidget*> pWindowList;
  /** "last" window status flags are used in eventFilter() to detect individual dialog
   * geometries resetting */
  QHash<QWidget*,bool> pLastWindowShownStatus;
  QHash<QWidget*,QRect> pLastWindowGeometries;
  /** "saved" window status flags are used in hideEvent() to save the individual dialog visible
   * states, as the "last" status flags will be overridden by all the windows hiding. */
  QHash<QWidget*,bool> pSavedWindowShownStatus;
  //  QHash<QWidget*, bool> _lastwindowshownstatus;
  //  QHash<QWidget*, QRect> _lastwindowgeometries;
  //  QHash<QWidget*, bool> _savedwindowshownstatus;

  QString _widgetstyle;
};

#endif

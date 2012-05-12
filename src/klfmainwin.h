/***************************************************************************
 *   file klfmainwin.h
 *   This file is part of the KLatexFormula Project.
 *   Copyright (C) 2011 by Philippe Faist
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
#include <QDialog>
#include <QMainWindow>
#include <QClipboard>
#include <QShortcut>

#include <klfbackend.h>

#include <klfguiutil.h>

#include <klflib.h>
#include <klfconfig.h>
#include <klflatexsymbols.h>



class KLFLibBrowser;
class KLFStyleManager;
class KLFSettings;
class KLFLatexSyntaxHighlighter;
class KLFLatexEdit;
class KLFCmdIface;
class KLFUserScriptSettings;


namespace Ui {
  class KLFProgErr;
  class KLFMainWin;
}

class KLFMainWin;


class KLF_EXPORT KLFProgErr : public QDialog
{
  Q_OBJECT
public:
  KLFProgErr(QWidget *parent, QString errtext);
  virtual ~KLFProgErr();

  static void showError(QWidget *parent, QString text);

protected:
  virtual void showEvent(QShowEvent *e);

private:
  Ui::KLFProgErr *u;
};


/** A helper interface class to implement more export formats to save output (to file).
 */
class KLFAbstractOutputSaver
{
public:
  KLFAbstractOutputSaver() { }
  virtual ~KLFAbstractOutputSaver() { }

  /** Returns a list of mime-types of supported file formats for the given output, which
   * may be NULL. In the latter case, this function reports all formats that it can in
   * principle provide. */
  virtual QStringList supportedMimeFormats(KLFBackend::klfOutput * klfoutput) = 0;

  /** Returns the human-readable, (possibly translated,) label to display in save dialog that
   * the user can select to save in this format.
   *
   * \param key is a mime-type returned by \ref supportedMimeFormats().
   */
  virtual QString formatTitle(const QString& key) = 0;

  /** Returns the file pattern(s) that the files of this format (normally) match.
   * syntax is simple pattern eg. \c "*.png".
   *
   * The patterns are joined to spaces to form a filter that is given to QFileDialog.
   */
  virtual QStringList formatFilePatterns(const QString& key) = 0;

  /** Actually save to the file \c fileName, using the format \c key.
   *
   * The subclass is responsible for notifying the user of possible errors that have occurred.
   *
   * Overwrite confirmation has already been required (if applicable).
   */
  virtual bool saveToFile(const QString& key, const QString& fileName, const KLFBackend::klfOutput& output) = 0;
};

/** A helper interface class to open old PNG files, library files or abstract data, to fill in
 * the main window controls (latex and style), or possibly open a resource into library.
 *
 * Instances of subclasses will be invoked when:
 *  - a file is given on command-line, see \ref canOpenFile() and \ref openFile()
 *  - data is pasted or dropped in editor, see \ref supportedMimeTypes(), \ref canOpenData() and
 *    \ref openData()
 *
 */
class KLF_EXPORT KLFAbstractDataOpener
{
public:
  KLFAbstractDataOpener(KLFMainWin *mainwin) : mMainWin(mainwin) { }
  virtual ~KLFAbstractDataOpener() { }

  /** Returns a list of mime-types we can handle */
  virtual QStringList supportedMimeTypes() = 0;

  /** Is supposed to peek into \c file to try to recognize if its format is one which we can open.
   * The implementation of this function may also rely on the file name's extension.
   *
   * If the file is recognized as one this opener can open, then return TRUE, otherwise return FALSE.
   */
  virtual bool canOpenFile(const QString& file) = 0;

  /** Is supposed to peek into \c data to try to recognize if its format is one which we can open.
   * No indication is given as to which format \c data is in. If the \c data is recognized as
   * a format this opener can open, return TRUE, otherwise, return FALSE. */
  virtual bool canOpenData(const QByteArray& data) = 0;

  /** Actually open the file. You may use the mainWin() to perform something useful.
   *
   * Note: this function will be called for every file the main window tries to open. Do NOT assume
   *   that the file given here is a file that passed the canOpenFile() function test. (Reason:
   *   calling both canOpenFile() and openFile() may result into resources being loaded twice, which
   *   is not optimal).
   *
   * This function should return FALSE if it is not capable of loading the given \c file.
   */
  virtual bool openFile(const QString& file) = 0;

  /** Actually open the data. You may use the mainWin() to perform something useful.
   *
   * \c mimetype is the mime-type of the data.
   *
   * Note: the \c mimetype can be empty, in which case the opener should make no assumption
   *   whatsoever as to the data's format, and try to parse data, and return FALSE if it
   *   is not capable of loading the given data. In particular, it should not be assumed
   *   that canOpenData() has already been called and returned true on this data.
   *
   * This function should return FALSE if it is not capable of loading the given \c data.
   */
  virtual bool openData(const QByteArray& data, const QString& mimetype) = 0;

protected:
  /** Get a pointer to the main window passed to the constructor. */
  KLFMainWin * mainWin() { return mMainWin; }

private:
  KLFMainWin *mMainWin;
};


class KLFLatexPreviewThread;
class KLFAboutDialog;
class KLFWhatsNewDialog;
class KLFMainWinPopup;

/**
 * KLatexFormula Main Window
 * \author Philippe Faist &lt;philippe.faist@bluewin.ch&gt;
 */
class KLF_EXPORT KLFMainWin : public QMainWindow, public KLFDropDataHandler
{
  Q_OBJECT
  Q_PROPERTY(QString widgetStyle READ widgetStyle WRITE setWidgetStyle)

public:
  KLFMainWin();
  virtual ~KLFMainWin();

  /** called by main.cpp right after show(), just before entering into event loop. */
  void startupFinished();

  bool eventFilter(QObject *obj, QEvent *event);

  bool isExpandedMode() const;

  KLFStyle currentStyle() const;

  KLFBackend::klfSettings backendSettings() const { return _settings; }

  virtual QFont txtLatexFont() const;
  virtual QFont txtPreambleFont() const;

  KLFBackend::klfSettings currentSettings() const { return _settings; }

  void applySettings(const KLFBackend::klfSettings& s);

  KLFBackend::klfOutput currentKLFBackendOutput() const { return _output; }

  KLFBackend::klfInput currentInputState() { return collectInput(false); }

  QString currentInputLatex() const;

  enum altersetting_which { altersetting_LBorderOffset = 100,
			    altersetting_TBorderOffset,
			    altersetting_RBorderOffset,
			    altersetting_BBorderOffset,
			    altersetting_TempDir,
			    altersetting_Latex,
			    altersetting_Dvips,
			    altersetting_Gs,
			    altersetting_Epstopdf,
			    altersetting_OutlineFonts, //!< bool given as an int value
			    altersetting_CalcEpsBoundingBox //!< bool given as an int value
  };

  KLFLibBrowser * libBrowserWidget() { return mLibBrowser; }
  KLFLatexSymbols * latexSymbolsWidget() { return mLatexSymbols; }
  KLFStyleManager * styleManagerWidget() { return mStyleManager; }
  KLFSettings * settingsDialog() { return mSettingsDialog; }
  QMenu * styleMenu() { return mStyleMenu; }
  KLFLatexEdit *latexEdit();
  KLFLatexSyntaxHighlighter * syntaxHighlighter();
  KLFLatexSyntaxHighlighter * preambleSyntaxHighlighter();

  KLFConfig * klfConfig() { return & klfconfig; }

  QString widgetStyle() const { return _widgetstyle; }

  void registerHelpLinkAction(const QString& path, QObject *object, const char * member, bool wantUrlParam);

  void registerOutputSaver(KLFAbstractOutputSaver *outputsaver);
  void unregisterOutputSaver(KLFAbstractOutputSaver *outputsaver);

  void registerDataOpener(KLFAbstractDataOpener *dataopener);
  void unregisterDataOpener(KLFAbstractDataOpener *dataopener);

  bool canOpenFile(const QString& fileName);
  bool canOpenData(const QByteArray& data);
  bool canOpenData(const QMimeData *mimeData);

  QList<KLFAbstractOutputSaver*> registeredOutputSavers() { return pOutputSavers; }
  QList<KLFAbstractDataOpener*> registeredDataOpeners() { return pDataOpeners; }

  //! Reimplemented from KLFDropDataHandler
  bool canOpenDropData(const QMimeData * data) { return canOpenData(data); }

  bool saveOutputToFile(const KLFBackend::klfOutput& output, const QString& fname, const QString& format,
			KLFAbstractOutputSaver * saver = NULL);


  bool isApplicationVisible() const;

signals:

  void evaluateFinished(const KLFBackend::klfOutput& output);

  void previewAvailable(const QImage& preview, const QImage& largePreview);
  void previewRealTimeError(const QString& errmsg, int errcode);

  // dialogs (e.g. stylemanager) should connect to this in case styles change unexpectedly
  void stylesChanged();

  void applicationLocaleChanged(const QString& newLocale);

  void klfConfigChanged();

  /** Emitted when view controls (eg. large preview button) are enabled/disabled */
  void userViewControlsActive(bool active);
  /** Emitted when save controls (eg. save button) are enabled/disabled */
  void userSaveControlsActive(bool active);

  void userActivity();

  void userInputChanged();

  /** emitted whenever the formula currently being edited is replaced by something else, like a restored
   * formula from the library or a blank input from clear button, etc.
   */
  void inputCleared();

  void aboutToCopyData();
  void copiedData(const QString& profile);
  void aboutToDragData();
  void draggedDataWasDropped(const QString& mimeType);
  void fileOpened(const QString& fname, KLFAbstractDataOpener * usingDataOpener);
  void dataOpened(const QString& mimetype, const QByteArray& data, KLFAbstractDataOpener * usingDataOpener);

  void aboutToSaveAs();
  /** \note if \c usingSaver is NULL, the built-in klfbackend saver was used */
  void savedToFile(const QString& fileName, const QString& format, KLFAbstractOutputSaver * usingSaver);

public slots:

  void slotEvaluate();
  void slotClear() { slotClearLatex(); }
  void slotClearLatex();
  void slotClearAll();
  void slotLibrary(bool showlib);
  void slotToggleLibrary();
  void slotLibraryButtonRefreshState(bool on);
  void slotSymbols(bool showsymbs = true);
  void slotToggleSymbols();
  void slotSymbolsButtonRefreshState(bool on);
  void slotExpandOrShrink();
  void slotExpand(bool expanded = true);
  void slotSetLatex(const QString& latex);
  void slotSetMathMode(const QString& mathmode);
  void slotSetPreamble(const QString& preamble);
  void slotSetUserScript(const QString& userScript);
  void slotShowLastUserScriptOutput();
  void slotReloadUserScripts();
  /** If \c line is already in the preamble, then does nothing. Otherwise appends
   * \c line to the preamble text. */
  void slotEnsurePreambleCmd(const QString& line);
  void slotSetDPI(int DPI);
  void slotSetFgColor(const QColor& fgcolor);
  void slotSetFgColor(const QString& fgcolor);
  void slotSetBgColor(const QColor& bgcolor);
  void slotSetBgColor(const QString& bgcolor);

  /** This function allows to temporarily modify a given setting with a new value. KLatexFormula
   * will NOT remember the new setting in later executions.
   *
   * Used eg. for command-line mode.
   *
   * Note you have to use the correct function for each setting, if the setting requires an int
   * use this function, if it requires a string use alterSetting(altersetting_which, QString).
   */
  void alterSetting(int altersettingWhich, int ivalue);
  /** See alterSetting(altersetting_which, int) */
  void alterSetting(int altersettingWhich, QString svalue);

  // will actually save only if output non empty.
  void slotEvaluateAndSave(const QString& output, const QString& format);

  void pasteLatexFromClipboard(QClipboard::Mode mode = QClipboard::Clipboard);

  bool openFile(const QString& file);
  bool openFiles(const QStringList& fileList);
  bool openData(const QMimeData *mimeData, bool *openerFound = NULL);
  //! Reimplemented from KLFDropDataHandler
  int openDropData(const QMimeData *mimeData);
  bool openData(const QByteArray& data);

  bool openLibFiles(const QStringList& files, bool showLibrary = true);
  bool openLibFile(const QString& file, bool showLibrary = true);

  void setApplicationLocale(const QString& locale);

  void retranslateUi(bool alsoBaseUi = true);

  bool loadDefaultStyle();
  bool loadNamedStyle(const QString& sty);

  void slotDrag();
  void slotCopy();
  void slotSave(const QString& suggestedFname = QString::null);
  void slotSetExportProfile(const QString& exportProfile);

  void slotActivateEditor();
  void slotActivateEditorSelectAll();

  void slotShowBigPreview();

  void slotPresetDPISender();
  void slotLoadStyle(int stylenum);
  void slotLoadStyle(const KLFStyle& style);
  void slotSaveStyle();
  void slotStyleManager();
  void slotSettings();


  void refreshWindowSizes();

  void refreshShowCorrectClearButton();

  void refreshExportTypesMenu();
  void refreshStylePopupMenus();
  void loadStyles();
  void loadLibrary(); // load library stuff
  void loadLibrarySavedState();
  void saveStyles();
  void restoreFromLibrary(const KLFLibEntry& entry, uint restoreflags);
  void insertSymbol(const KLFLatexSymbol& symbol);
  /** Inserts a delimiter \c delim, and brings the cursor \c charsBack characters
   * back. Eg. you can insert \c "\mathrm{}" and bring the cursor 1 space back. */
  void insertDelimiter(const QString& delim, int charsBack = 1);
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

  void setMacBrushedMetalLook(bool metallook);

  void showRealTimeReset();
  void showRealTimePreview(const QImage& preview, const QImage& largePreview);
  void showRealTimeError(const QString& errorstr, int errcode);

  void updatePreviewThreadInput();

  void displayError(const QString& errormsg);

  void refreshAllWindowStyleSheets();

  void setQuitOnClose(bool quitOnClose);

  void hideApplication();

  bool executeURLCommandsFromFile(const QString& fname);

  void quit();

private slots:
  // private : only as slot to an action containing the style # as user data
  void slotLoadStyleAct();

  void slotDetailsSideWidgetShown(bool shown);

  /** controls the enabled state of the 'see larger preview button' widget */
  void slotSetViewControlsEnabled(bool enabled);
  /** controls the enabled state of the DRAG/COPY/SAVE & Format widgets */
  void slotSetSaveControlsEnabled(bool enabled);

  void slotOpenHistoryLibraryResource();

  void slotNewSymbolTyped(const QString& symbol);
  void slotPopupClose();
  void slotPopupAction(const QUrl& helpLinkUrl);
  void slotPopupAcceptAll();

  void slotEditorContextMenuInsertActions(const QPoint& pos, QList<QAction*> *actionList);
  void slotInsertMissingPackagesFromActionSender();
  void slotChangeParenFromActionSender();
  void slotChangeParenFromVariantMap(const QVariantMap& data);

  void slotCycleParenModifiers(bool forward = true);
  void slotCycleParenModifiersBack() { slotCycleParenModifiers(false); }
  void slotCycleParenTypes(bool forward = true);
  void slotCycleParenTypesBack() { slotCycleParenTypes(false); }

  void slotEncloseRegionInDelimiterFromActionSender();
  void slotEncloseRegionInDelimiter(const QVariantMap& vmap);

  void latexEditReplace(int pos, int len, const QString& text);

  void slotUserScriptSet(int index);

protected:
  Ui::KLFMainWin *u;

  KLFLibBrowser *mLibBrowser;
  KLFLatexSymbols *mLatexSymbols;
  KLFStyleManager *mStyleManager;
  KLFSettings *mSettingsDialog;
  KLFAboutDialog *mAboutDialog;
  KLFWhatsNewDialog *mWhatsNewDialog;

  KLFMainWinPopup *mPopup;

  QShortcut * mShortcutNextParenType;
  QShortcut * mShortcutNextParenModifierType;

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

  KLFStyleList _styles;

  bool try_load_style_list(const QString& fileName);

  QMenu *mStyleMenu;

  bool _loadedlibrary;
  bool _firstshow;

  KLFBackend::klfSettings _settings; // settings we pass to KLFBackend
  bool _settings_altered;

  KLFBackend::klfOutput _output; // output from KLFBackend
  //  KLFBackend::klfInput _lastrun_input; // input that generated _output (now _output.input)

  /** If TRUE, then the output contained in _output is up-to-date, meaning that we favor displaying
   * _output.result instead of the image given by mPreviewBuilderThread. */
  bool _evaloutput_uptodate;
  /** The Thread that will create real-time previews of formulas. */
  KLFLatexPreviewThread *pLatexPreviewThread;

  QLabel *mExportMsgLabel;
  void showExportMsgLabel(const QString& msg, int timeout = 3000);
  int pExportMsgLabelTimerId;

  KLFUserScriptSettings * pUserScriptSettings;

  /**
   * Use \ref currentInputState() instead for "public" use.
   *
   * Returns the input corresponding to the current GUI state. If \c isFinal is TRUE, then
   * the input data may be "remembered" as used (the exact effect depends on the setting), eg.
   * math mode is memorized into combo box choices. Typically \c isFinal is TRUE when called
   * from slotEvaluate() and FALSE when called to update the preview builder thread. */
  KLFBackend::klfInput collectInput(bool isFinal);

  QList<QAction*> pExportProfileQuickMenuActionList;

  QSize _shrinkedsize;
  QSize _expandedsize;

  bool event(QEvent *e);
#ifdef Q_WS_X11
  bool x11Event(XEvent *event);
#endif
  void childEvent(QChildEvent *e);
  void closeEvent(QCloseEvent *e);
  void hideEvent(QHideEvent *e);
  void showEvent(QShowEvent *e);
  void timerEvent(QTimerEvent *e);

  bool _ignore_close_event;

  //   /** "last" window status flags are used in eventFilter() to detect individual dialog
  //    * geometries resetting */
  //   QHash<QWidget*,bool> pLastWindowShownStatus;
  //   QHash<QWidget*,QRect> pLastWindowGeometries;
  //   /** "saved" window status flags are used in hideEvent() to save the individual dialog visible
  //    * states, as the "last" status flags will be overridden by all the windows hiding. */
  //   QHash<QWidget*,bool> pSavedWindowShownStatus;

  QString _widgetstyle;

  void getMissingCmdsFor(const QString& symbol, QStringList * missingCmds, QString *guiText,
			 bool wantHtmlText = true);

  QList<KLFAbstractOutputSaver*> pOutputSavers;
  QList<KLFAbstractDataOpener*> pDataOpeners;

  KLFCmdIface *pCmdIface;

  QVariantMap parseLatexEditPosParenInfo(KLFLatexEdit *editor, int pos);
};

#endif

#ifndef EDITOR_H
#define EDITOR_H

#include "include/EditorNS/customqwebview.h"
#include "include/promise.h"
#include <QObject>
#include <QVariant>
#include <QQueue>
#include <QWheelEvent>
#include <QVBoxLayout>
#include <QTextCodec>
#include <QVariant>
#include <functional>
#include <future>

class EditorTabWidget;

namespace EditorNS
{

    /**
         * @brief An Object injectable into the javascript page, that allows
         *        the javascript code to send messages to an Editor object.
         *        It also allows the js instance to retrieve message data information.
         *
         * Note that this class is only needed for the current Editor
         * implementation, that uses QWebView.
         */
    class JsToCppProxy : public QObject
    {
        Q_OBJECT

    private:
        QVariant m_msgData;

    public:
        JsToCppProxy(QObject *parent) : QObject(parent) { }

        Q_INVOKABLE void receiveMessage(QString msg, QVariant data) { emit messageReceived(msg, data); }

    signals:
        /**
             * @brief A JavaScript message has been received.
             * @param msg Message type
             * @param data Message data
             */
        void messageReceived(QString msg, QVariant data);

        void messageReceivedByJs(QString msg, QVariant data);
    };


    /**
         * @brief Provides a JavaScript CodeMirror instance.
         *
         * Communication works by sending messages to the javascript Editor using
         * the sendMessage() method. On the other side, when a javascript event
         * occurs, the messageReceived() signal will be emitted.
         *
         * In addition to messageReceived(), other signals could be emitted at the
         * same time, for example currentLineChanged(). This is simply for
         * convenience, so that the user of this class doesn't need to manually parse
         * the arguments for pre-defined messages.
         *
         */
    class Editor : public QWidget
    {
        Q_OBJECT
    public:

        struct Theme {
            QString name;
            QString path;
        };

        explicit Editor(const Theme &theme, QWidget *parent = 0);
        explicit Editor(QWidget *parent = 0);

        /**
             * @brief Efficiently returns a new Editor object from an internal buffer.
             * @return
             */
        static QSharedPointer<Editor> getNewEditor(QWidget *parent = 0);
        static Editor *getNewEditorUnmanagedPtr(QWidget *parent);

        static void invalidateEditorBuffer();

        struct LanguageGreater {
            inline bool operator()(const QMap<QString, QString> &v1, const QMap<QString, QString> &v2) const {
                return v1.value("name").toLower() < v2.value("name").toLower();
            }
        };

        struct Cursor {
            int line;
            int column;

            bool operator == (const Cursor &x) const {
                return line == x.line && column == x.column;
            }

            bool operator < (const Cursor &x) const {
                return std::tie(line, column) < std::tie(x.line, x.column);
            }

            bool operator <= (const Cursor &x) const {
                return *this == x || *this < x;
            }

            bool operator > (const Cursor &x) const {
                return !(*this <= x);
            }

            bool operator >= (const Cursor &x) const {
                return !(*this < x);
            }
        };

        struct Selection {
            Cursor from;
            Cursor to;
        };

        struct IndentationMode {
            bool useTabs;
            int size;
        };

        /**
             * @brief Adds a new Editor to the internal buffer used by getNewEditor().
             *        You might want to call this method e.g. as soon as the application
             *        starts (so that an Editor is ready as soon as it gets required),
             *        or when the application is idle.
             * @param howMany specifies how many Editors to add
             * @return
             */
        static void addEditorToBuffer(const int howMany = 1);

        /**
             * @brief Give focus to the editor, so that the user can start
             *        typing. Note that calling won't automatically switch to
             *        the tab where the editor is. Use EditorTabWidget::setCurrentIndex()
             *        and TopEditorContainer::setFocus() for that.
             */
        Q_INVOKABLE void setFocus();

        /**
             * @brief Remove the focus from the editor.
             */
        Q_INVOKABLE void clearFocus();

        /**
             * @brief Set the file name associated with this editor
             * @param filename full path of the file
             */
        Q_INVOKABLE void setFilePath(const QUrl &filename);

        /**
             * @brief Get the file name associated with this editor
             * @return
             */
        Q_INVOKABLE QUrl filePath() const;

        Q_INVOKABLE bool fileOnDiskChanged() const;
        Q_INVOKABLE void setFileOnDiskChanged(bool fileOnDiskChanged);

        enum selectMode {
            selectMode_cursorBefore,
            selectMode_cursorAfter,
            selectMode_selected
        };

        void insertBanner(QWidget *banner);
        void removeBanner(QWidget *banner);
        void removeBanner(QString objectName);

        // Lower-level message wrappers:
        Q_INVOKABLE Promise<bool> isClean();
        Q_INVOKABLE void markClean();
        Q_INVOKABLE void markDirty();
        QList<QMap<QString, QString> > languages();

        /**
         * @brief Set the language to use for the editor.
         *        It automatically adjusts tab settings from
         *        the default configuration for the specified language.
         * @param language Language id
         */
        Q_INVOKABLE void setLanguage(const QString &language);
        Q_INVOKABLE QString setLanguageFromFileName(QString filePath);
        Q_INVOKABLE QString setLanguageFromFileName();
        Q_INVOKABLE void setValue(const QString &value);
        Q_INVOKABLE QString value();

        /**
         * @brief Set custom indentation settings which may be different
         *        from the default tab settings associated with the current
         *        language.
         *        If this method is called, further calls to setLanguage()
         *        will NOT modify these tab settings. Use
         *        clearCustomIndentationMode() to reset to default settings.
         * @param useTabs
         * @param size Size of an indentation. If 0, keeps the current one.
         */
        void setCustomIndentationMode(const bool useTabs, const int size);
        void setCustomIndentationMode(const bool useTabs);
        void clearCustomIndentationMode();
        bool isUsingCustomIndentationMode() const;

        Q_INVOKABLE void setSmartIndent(bool enabled);
        Q_INVOKABLE qreal zoomFactor() const;
        Q_INVOKABLE void setZoomFactor(const qreal &factor);
        Q_INVOKABLE void setSelectionsText(const QStringList &texts, selectMode mode);
        Q_INVOKABLE void setSelectionsText(const QStringList &texts);
        Q_INVOKABLE QString language();
        Q_INVOKABLE void setLineWrap(const bool wrap);
        Q_INVOKABLE void setEOLVisible(const bool showeol);
        Q_INVOKABLE void setWhitespaceVisible(const bool showspace);

        /**
         * @brief Get the current cursor position
         * @return a <line, column> pair.
         */
        QPair<int, int> cursorPosition();
        void setCursorPosition(const int line, const int column);
        void setCursorPosition(const QPair<int, int> &position);
        void setCursorPosition(const Cursor &cursor);

        /**
         * @brief Get the current scroll position
         * @return a <left, top> pair.
         */
        QPair<int, int> scrollPosition();
        void setScrollPosition(const int left, const int top);
        void setScrollPosition(const QPair<int, int> &position);
        QString endOfLineSequence() const;
        void setEndOfLineSequence(const QString &endOfLineSequence);

        /**
         * @brief Applies a font family/size to the Editor.
         * @param fontFamily the family to be applied. An empty string or
         *                   nullptr denote no override.
         * @param fontSize the size to be applied. 0 denotes no override.
         */
        void setFont(QString fontFamily, int fontSize, double lineHeight);

        QTextCodec *codec() const;

        /**
         * @brief Set the codec for this Editor.
         *        This method does not change the in-memory or on-screen
         *        representation of the document (which is always Unicode).
         *        It serves solely as a way to keep track of the encoding
         *        that needs to be used when the document gets saved.
         * @param codec
         */
        void setCodec(QTextCodec *codec);

        bool bom() const;
        void setBom(bool bom);

        QList<Theme> themes();
        void setTheme(Theme theme);
        static Editor::Theme themeFromName(QString name);

        QList<Selection> selections();

        /**
         * @brief Returns the currently selected texts.
         * @return
         */
        Q_INVOKABLE QStringList selectedTexts();

        void setOverwrite(bool overwrite);
        void setTabsVisible(bool visible);

        /**
         * @brief Detect the indentation mode used within the current document.
         * @return
         */
        Editor::IndentationMode detectDocumentIndentation(bool *found = nullptr);
        Editor::IndentationMode indentationMode();

        QString getCurrentWord();

        void setSelection(int fromLine, int fromCol, int toLine, int toCol);

        int lineCount();

    private:
        friend class ::EditorTabWidget;

        struct AsyncReply {
            unsigned int id;
            QString message;
            Promise<QVariant> value;
            std::function<void (QVariant)> callback;
        };

        std::list<AsyncReply> asyncReplies;

        // These functions should only be used by EditorTabWidget to manage the tab's title. This works around
        // KDE's habit to automatically modify QTabWidget's tab titles to insert shortcut sequences (like &1).
        QString tabName() const;
        void setTabName(const QString& name);

        static QQueue<Editor*> m_editorBuffer;
        QVBoxLayout *m_layout;
        CustomQWebView *m_webView;
        JsToCppProxy *m_jsToCppProxy;
        QUrl m_filePath = QUrl();
        QString m_tabName;
        bool m_fileOnDiskChanged = false;
        bool m_loaded = false;
        QString m_endOfLineSequence = "\n";
        QTextCodec *m_codec = QTextCodec::codecForName("UTF-8");
        bool m_bom = false;
        bool m_customIndentationMode = false;

        inline void waitAsyncLoad();
        QString jsStringEscape(QString str) const;

        void fullConstructor(const Theme &theme);

        void setIndentationMode(const bool useTabs, const int size);
        void setIndentationMode(QString language);

    private slots:
        void on_proxyMessageReceived(QString msg, QVariant data);

    signals:
        void messageReceived(QString msg, QVariant data);
        void asyncReplyReceived(unsigned int id, QString msg, QVariant data);
        void gotFocus();
        void mouseWheel(QWheelEvent *ev);
        void urlsDropped(QList<QUrl> urls);
        void bannerRemoved(QWidget *banner);

        // Pre-interpreted messages:
        void contentChanged();
        void cursorActivity();
        void cleanChanged(bool isClean);
        void fileNameChanged(const QUrl &oldFileName, const QUrl &newFileName);

        /**
             * @brief The editor finished loading. There should be
             *        no need to use this signal outside this class.
             */
        void editorReady();

        void currentLanguageChanged(QString id, QString name);

    public slots:
        void sendMessage(const QString &msg, const QVariant &data);
        void sendMessage(const QString &msg);

        /**
         * @brief asyncSendMessageWithResult
         * @param msg
         * @param data
         * @param callback When set, the result is returned asynchronously via the provided function.
         *                 If set, you should NOT use the return value of this method.
         * @return
         */
        Promise<QVariant> asyncSendMessageWithResult(const QString &msg, const QVariant &data, std::function<void(QVariant)> callback = 0);
        Promise<QVariant> asyncSendMessageWithResult(const QString &msg, std::function<void(QVariant)> callback = 0);

        void print(std::shared_ptr<QPrinter> printer);
    };

}

#endif // EDITOR_H

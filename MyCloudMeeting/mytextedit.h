#ifndef MYTEXTEDIT_H
#define MYTEXTEDIT_H

#include <QObject>
#include <QCompleter>
#include <QWidget>
#include <QPlainTextEdit>
#include <QVBoxLayout>
#include <QStringListModel>
#include <QScopedPointer>
#include <QAbstractItemView>
#include <QScrollBar>

//用于提供自动完成功能的类
// ?? 没看懂
class Completer : public QCompleter
{
    Q_OBJECT
public:
    explicit Completer(QWidget* parent=nullptr);
};


// 编辑文字
class MyTextEdit : public QWidget
{
    Q_OBJECT
public:
    explicit MyTextEdit(QWidget* parent=nullptr);

public:
    QString toPlainText();
    void setPlainText(QString);
    void setPlaceholderText(QString);
    void setCompleter(const QStringList& );
private:
    QString textUnderCursor();
    bool eventFilter(QObject *, QEvent *);
private:
    QPlainTextEdit *m_edit;
    Completer *m_completer;
    QVector<QPair<int, int> > m_ipspan;
private slots:
    void changeCompletion(QString);
public slots:
    void complete();
};

#endif // MYTEXTEDIT_H

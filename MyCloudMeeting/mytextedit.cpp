#include "mytextedit.h"


Completer::Completer(QWidget *parent):QCompleter(parent)
{

}

MyTextEdit::MyTextEdit(QWidget *parent) :QWidget(parent)
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_edit = new QPlainTextEdit();
    m_edit->setPlaceholderText(QString::fromUtf8("&#x8F93;&#x5165;@&#x53EF;&#x4EE5;&#x5411;"
                                               "&#x5BF9;&#x5E94;&#x7684;IP&#x53D1;&#x9001;&#x6570;&#x636E;"));
    layout->addWidget(m_edit);
    m_completer = nullptr;
    connect(m_edit, SIGNAL(textChanged()), this, SLOT(complete()));
}

QString MyTextEdit::toPlainText()
{
    return m_edit->toPlainText();
}

void MyTextEdit::setPlainText(QString str)
{
    m_edit->setPlainText(str);
}

void MyTextEdit::setPlaceholderText(QString str)
{
    m_edit->setPlaceholderText(str);
}

// 自动补全
void MyTextEdit::setCompleter(const QStringList& stringList)
{
    if (!m_completer) {
        m_completer = new Completer(this);
        m_completer->setWidget(this);
        m_completer->setCompletionMode(QCompleter::PopupCompletion);// 弹出菜单模式
        m_completer->setCaseSensitivity(Qt::CaseInsensitive);
        connect(m_completer, SIGNAL(activated(QModelIndex)), this, SLOT(changeCompletion(QString)));
    } else {
        QStringListModel* model = qobject_cast<QStringListModel*>(m_completer->model());
        if (model) {
            model->setStringList(stringList);
        } else {
            delete m_completer->model();
            model = new QStringListModel(stringList, this);
            m_completer->setModel(model);
        }
    }
}


QString MyTextEdit::textUnderCursor()
{
    QTextCursor cursor = m_edit->textCursor();
    cursor.select(QTextCursor::WordUnderCursor);
    return cursor.selectedText();
}


// 将选择范围内的文字删除？？？
bool MyTextEdit::eventFilter(QObject *obj, QEvent *ev)
{
    if (obj == m_edit && ev->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(ev);
        QTextCursor cursor = m_edit->textCursor();
        int cursorPosition = cursor.position();

        for (auto it = m_ipspan.begin(); it != m_ipspan.end(); ++it) {
            int rangeStart = it->first;
            int rangeEnd = it->second;

            if ((keyEvent->key() == Qt::Key_Backspace && cursorPosition > rangeStart && cursorPosition <= rangeEnd) ||
                (keyEvent->key() == Qt::Key_Delete && cursorPosition >= rangeStart && cursorPosition < rangeEnd)) {
                cursor.setPosition(rangeStart);
                cursor.setPosition(rangeEnd, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
                it = m_ipspan.erase(it); // 使用迭代器安全地删除元素
                return true;
            } else if (cursorPosition >= rangeStart && cursorPosition <= rangeEnd) {
                cursor.setPosition(rangeEnd);
                m_edit->setTextCursor(cursor);
                return true;
            }
        }
    }
    return false;
}

// 用于在文本编辑器中插入自动完成的文本
void MyTextEdit::changeCompletion(QString text)
{
    QTextCursor tc = m_edit->textCursor();
    int len = text.size() - m_completer->completionPrefix().size();
    tc.movePosition(QTextCursor::EndOfWord);
    tc.insertText(text.right(len));
    m_edit->setTextCursor(tc);
    m_completer->popup()->hide();

    QString str = m_edit->toPlainText();
    int pos = str.size() - 1;
    while(str.at(pos) != '@') pos--;

    tc.clearSelection();
    tc.setPosition(pos, QTextCursor::MoveAnchor);
    tc.setPosition(str.size(), QTextCursor::KeepAnchor);
      // tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, str.size() - pos);

    QTextCharFormat fmt = tc.charFormat();
    QTextCharFormat fmt_back = fmt;
    fmt.setForeground(QBrush(Qt::white));
    fmt.setBackground(QBrush(QColor(0, 160, 233)));
    tc.setCharFormat(fmt);
    tc.clearSelection();
    tc.setCharFormat(fmt_back);

    tc.insertText(" ");
    m_edit->setTextCursor(tc);

    m_ipspan.push_back(QPair<int, int>(pos, str.size()+1));
}

void MyTextEdit::complete()
{
    if(m_edit->toPlainText().size() == 0 || m_completer == nullptr) return;
    QChar tail =  m_edit->toPlainText().at(m_edit->toPlainText().size()-1);
    if(tail == '@')
    {
        m_completer->setCompletionPrefix(tail);
        QAbstractItemView *view = m_completer->popup();
        view->setCurrentIndex(m_completer->completionModel()->index(0, 0));
        QRect cr = m_edit->cursorRect();
        QScrollBar *bar = m_completer->popup()->verticalScrollBar();
        cr.setWidth(m_completer->popup()->sizeHintForColumn(0) + bar->sizeHint().width());
        m_completer->complete(cr);
    }
}


#ifndef PARTNER_H
#define PARTNER_H

#include <QObject>
#include <QLabel>
#include <QHostAddress>


//用于记录房间用户
class Partner : public QLabel
{
    Q_OBJECT
public:
    explicit Partner(QWidget *parent = nullptr, quint32 ip=0);
    void setPix(QImage img);
private:
    quint32 m_ip;
    int m_width;
    void mousePressEvent(QMouseEvent *ev) override;

signals:
    void sendIp(quint32);
};

#endif // PARTNER_H

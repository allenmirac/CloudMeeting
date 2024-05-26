#include "partner.h"

Partner::Partner(QWidget *parent, quint32 ip) : QLabel(parent)
{
    this->m_ip = ip;
    //水平方向扩展，垂直方向为最小的
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);//设置窗口大小策略

    //将图片按指定大小显示在矩形边框中
    m_width = ((QWidget*)this->parent())->size().width();
    this->setPixmap(QPixmap::fromImage(QImage(":/img/2.jpg").scaled(m_width-10, m_width-10)));
    this->setFrameShape(QFrame::Box);

    //为当前部件显示一个矩形边框，
    this->setStyleSheet("border-width: 1px; border-style: solid; border-color:rgba(0, 0 , 255, 0.7)");

    //设置部件工具提示词
    this->setToolTip(QHostAddress(m_ip).toString());

}

void Partner::setPix(QImage img)
{
    this->setPixmap(QPixmap::fromImage(img.scaled(m_width-10, m_width-10)));
}

void Partner::mousePressEvent(QMouseEvent *)
{
    emit sendIp(m_ip);
}


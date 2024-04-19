#ifndef MYVIDEOSURFACE_H
#define MYVIDEOSURFACE_H

#include <QObject>
#include <QAbstractVideoSurface>
#include <QVideoSurfaceFormat>

class MyVideoSurface : public QAbstractVideoSurface
{
    Q_OBJECT
public:
    explicit MyVideoSurface(QObject *parent = nullptr);
    QList<QVideoFrame::PixelFormat> supportedPixelFormats(QAbstractVideoBuffer::HandleType handleType = QAbstractVideoBuffer::NoHandle) const override;

    //检测视频流的格式是否合法，返回bool
    bool isFormatSupported(const QVideoSurfaceFormat &format) const override;
    bool start(const QVideoSurfaceFormat &format) override;
    bool present(const QVideoFrame &frame) override;
    QRect videoRect() const;
    void updateVideoRect();
    void paint(QPainter *painter);
signals:
    void frameAvailable(QVideoFrame);

};

#endif // MYVIDEOSURFACE_H

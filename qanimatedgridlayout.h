#ifndef QANIMATEDGRIDLAYOUT_H
#define QANIMATEDGRIDLAYOUT_H

#include <QLayout>

class QAnimatedGridLayoutPrivate;
class QEasingCurve;

class QAnimatedGridLayout : public QLayout
{
    Q_OBJECT
public:
    explicit QAnimatedGridLayout(QWidget *parent = 0);
    ~QAnimatedGridLayout();

    virtual void addItem(QLayoutItem *item);
    virtual void setGeometry(const QRect &rect);
    virtual QSize sizeHint() const;
    virtual QLayoutItem *itemAt(int index) const;
    virtual QLayoutItem *takeAt(int index);
    virtual int count() const;
    QSize minimumSize() const;
    
    bool hasHeightForWidth();
    bool isEmpty();
    void invalidate();

    void addWidget(QWidget *widget, int row, int column,
                   int rowSpan = 1, int colSpan = 1);
    int rowCount();
    int columnCount();

    void setEasingCurve(const QEasingCurve &easing);
    QEasingCurve easingCurve() const;

    void setAnimationDuration(int duration);
    int animationDuration();

    bool isZoomed();
    QWidget * zoomedWidget();

    void setVisibleAreaWidget(QWidget * widget); // to move out of this

signals:
    void animationFinished();
    
public slots:
    void zoomTo(QWidget *widget);
    void showAll();

private:
    // Q_DISABLE_COPY
    QAnimatedGridLayout(const QAnimatedGridLayout &);
    QAnimatedGridLayout &operator=(const QAnimatedGridLayout &);

    QAnimatedGridLayoutPrivate * m_private;
    friend class QAnimatedGridLayoutPrivate;

private slots:
    void animationFinishedSlot();
};

#endif // QANIMATEDGRIDLAYOUT_H

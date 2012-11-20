
#include <QApplication>
#include <QWidget>
#include <QVector>
#include <QEasingCurve>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include "qanimatedgridlayout.h"

class QAnimatedGridLayoutPrivate
{
    friend class QAnimatedGridLayout;
    QAnimatedGridLayout * m_public;
    struct Wrapper
    {
        Wrapper(QWidgetItem * i, int r, int c, int rs, int cs)
            { item = i; row = r; col = c; rowSpan = rs; colSpan = cs; }
        ~Wrapper()
            { if (item) delete item; }
        QWidgetItem * item;
        int row;
        int col;
        int rowSpan;
        int colSpan;
    };
    QList<Wrapper*> list;
    int rowCount;
    int colCount;
    QEasingCurve easing;
    bool dirty;
    QSize minSize;  // cache
    QSize hintedSize;   // cache
    int zoomedIndex;
    int duration;
    bool animationRunning;
    QWidget * outerWidgetForRect;
public:
    QAnimatedGridLayoutPrivate(QAnimatedGridLayout * public_)
        : m_public(public_)
    {
        rowCount = 0;
        colCount = 0;
        dirty = true;
        zoomedIndex = -1;
        duration = 250;
        animationRunning = false;
        outerWidgetForRect = NULL;
    }
    ~QAnimatedGridLayoutPrivate()
    {
        deleteAll();
    }
    void deleteAll()
    {
        while (!list.isEmpty())
            delete list.takeFirst();
        rowCount = 0;
        colCount = 0;
        zoomedIndex = -1;
    }
    void addItem(QLayoutItem *i, int r, int c, int rs, int cs)
    {
        Q_ASSERT(false);
        Q_UNUSED(i);
        Q_UNUSED(r);
        Q_UNUSED(c);
        Q_UNUSED(rs);
        Q_UNUSED(cs);
    }
    void addItem(QWidgetItem *i, int r, int c, int rs, int cs)
    {
        // TODO : check multiple add to same location !!!
        Q_CHECK_PTR(i);
        Q_ASSERT(r>=0);
        Q_ASSERT(c>=0);
        Q_ASSERT(rs>=1);
        Q_ASSERT(cs>=1);
        list.append(new Wrapper(i,r,c,rs,cs));
        int maxRowCount = (r+(rs-1))+1;
        int maxColCount = (c+(cs-1))+1;
        if (rowCount<maxRowCount) rowCount = maxRowCount;
        if (colCount<maxColCount) colCount = maxColCount;
    }
    QLayoutItem *itemAt(int i) const
    {
        if (0 <= i && i < list.size())
            return list.at(i)->item;
        else
            return NULL;
    }
    QLayoutItem *takeAt(int i)
    {
        bool aboutToTakeSOI = false;
        if (i == zoomedIndex)
            aboutToTakeSOI = true;
        Wrapper * w = list.takeAt(i);
        if (!w) return NULL;
        if (aboutToTakeSOI) zoomedIndex = -1;
        QWidgetItem * item = w->item;
        delete w;
        return item;
    }
    QSize minimumSize()
    {
        if (!dirty)
            return minSize;
        QVector<int> rowWidthTotals(rowCount,0);
        QVector<int> colHeightTotals(colCount,0);
        for (int i = 0; i < list.size(); ++i) {
            Wrapper * wr = list.at(i);
            int spacing = m_public->spacing();
            rowWidthTotals[wr->row] += wr->item->minimumSize().width() + spacing;
            colHeightTotals[wr->col] += wr->item->minimumSize().height() + spacing;
        }
        int h = 0;
        int w = 0;
        for (int i = 0; i < rowWidthTotals.size(); ++i) {
            w = rowWidthTotals[i] > w ? rowWidthTotals[i] : w;
        }
        for (int i = 0; i < colHeightTotals.size(); ++i) {
            h = colHeightTotals[i] > h ? colHeightTotals[i] : h;
        }
        minSize = QSize(w,h);
        return minSize;
    }
    QSize sizeHint()
    {
        if (!dirty)
            return hintedSize;
        QVector<int> rowWidthTotals(rowCount,0);
        QVector<int> colHeightTotals(colCount,0);
        for (int i = 0; i < list.size(); ++i) {
            Wrapper * wr = list.at(i);
            int spacing = m_public->spacing();
            rowWidthTotals[wr->row] += wr->item->sizeHint().width() + spacing;
            colHeightTotals[wr->col] += wr->item->sizeHint().height() + spacing;
        }
        int h = 0;
        int w = 0;
        for (int i = 0; i < rowWidthTotals.size(); ++i) {
            w = rowWidthTotals[i] > w ? rowWidthTotals[i] : w;
        }
        for (int i = 0; i < colHeightTotals.size(); ++i) {
            h = colHeightTotals[i] > h ? colHeightTotals[i] : h;
        }
        hintedSize = QSize(w,h);
        return hintedSize;
    }
    void setGeometry(const QRect & adjustedGeo)
    {
        if (animationRunning)
            return;
        if (!rowCount || !colCount)
            return;
        if (zoomedIndex >= 0) {
            Wrapper * wr = list.at(zoomedIndex);
            wr->item->setGeometry(adjustedGeo);
            return;
        }
        int spacing = m_public->spacing();
        QSize cellSize = calculateCellSize(adjustedGeo,spacing);
        for (int i = 0; i < list.size(); ++i) {
            Wrapper * wr = list.at(i);
            int r = wr->row,c = wr->col,rs = wr->rowSpan,cs = wr->colSpan;
            wr->item->setGeometry(
                        calculateCellGeometry(
                            adjustedGeo, cellSize, spacing, r,c,rs,cs));
        }
    }
    void startZoomInAnimation(QWidget * widget)
    {
        if (zoomedIndex >= 0)
            return;

        zoomedIndex = widgetIndex(widget);
        if (zoomedIndex < 0) // widget is not in the list
            return;

        QRect winGeo = m_public->geometry();
        if (outerWidgetForRect)
            winGeo = outerWidgetForRect->geometry();
        int lm, tm, rm, bm;
        m_public->getContentsMargins(&lm, &tm, &rm, &bm);
        QRect adjustedGeo = m_public->geometry().adjusted(lm, tm, -rm, -bm);
        int spacing = m_public->spacing();

        QSize cellSize = calculateCellSize(adjustedGeo,spacing);
        QParallelAnimationGroup * animGroup = new QParallelAnimationGroup;
        for (int i = 0; i < list.size(); ++i) {
            Wrapper * wr = list.at(i);
            QPropertyAnimation * anim = new QPropertyAnimation(wr->item->widget(), "pos");
            if (wr->item->widget() == widget)
                continue;
            anim->setEndValue(calculateOutPos(wr,winGeo,cellSize,spacing));
            anim->setDuration(duration);
            anim->setEasingCurve(easing);
            animGroup->addAnimation(anim);
            qApp->connect(anim,SIGNAL(finished()),wr->item->widget(),SLOT(hide()));
        }
        QPropertyAnimation * anim = new QPropertyAnimation(widget, "geometry");
        anim->setEndValue(adjustedGeo);
        anim->setDuration(duration);
        anim->setEasingCurve(easing);
        animGroup->addAnimation(anim);
        qApp->connect(animGroup, SIGNAL(finished()),m_public, SIGNAL(animationFinished()));
        animationRunning = true;
        animGroup->start(QAbstractAnimation::DeleteWhenStopped);
    }
    void startZoomOutAnimation()
    {
        if (zoomedIndex < 0)
            return;

        int lm, tm, rm, bm;
        m_public->getContentsMargins(&lm, &tm, &rm, &bm);
        QRect adjustedGeo = m_public->geometry().adjusted(lm, tm, -rm, -bm);
        int spacing = m_public->spacing();

        QSize cellSize = calculateCellSize(adjustedGeo,spacing);
        QParallelAnimationGroup * animGroup = new QParallelAnimationGroup;
        for (int i = 0; i < list.size(); ++i) {
            Wrapper * wr = list.at(i);
            wr->item->widget()->show();
            QPropertyAnimation * anim = new QPropertyAnimation(wr->item->widget(), "geometry");
            anim->setEndValue(
                        calculateCellGeometry(
                            adjustedGeo, cellSize, spacing,
                            wr->row, wr->col, wr->rowSpan,wr->colSpan));
            anim->setDuration(duration);
            anim->setEasingCurve(easing);
            animGroup->addAnimation(anim);
        }
        qApp->connect(animGroup, SIGNAL(finished()), m_public, SIGNAL(animationFinished()));
        animationRunning = true;
        animGroup->start(QAbstractAnimation::DeleteWhenStopped);
        zoomedIndex = -1;
    }
    void setAnimationFinished()
    {
        animationRunning = false;
    }
    int widgetIndex(QWidget * widget)
    {
        for (int i = 0; i < list.size(); ++i) {
            Wrapper * wr = list.at(i);
            if (wr->item->widget() == widget) {
                return i;
            }
        }
        return -1;
    }
    QWidget * zoomedTo()
    {
        if (zoomedIndex < 0)
            return NULL;
        Wrapper * wr = list.at(zoomedIndex);
        return wr->item->widget();
    }

private:
    /* Also uses zoomedIndex ! */
    QPoint calculateOutPos(Wrapper * wr, const QRect & winGeo,
                           const QSize & cellSize, int spacing)
    {
        Wrapper * currWr = list.at(zoomedIndex);
        if (wr->item->widget() == currWr->item->widget())
            return QPoint(0,0); // unused
        QRect geo = calculateCellGeometry(
                    winGeo, cellSize, spacing, wr->row, wr->col,
                    wr->rowSpan,wr->colSpan);
        QRect currGeo = calculateCellGeometry(
                    winGeo, cellSize, spacing, currWr->row, currWr->col,
                    currWr->rowSpan,currWr->colSpan);
        int x = 0, y = 0;
        if (wr->row < currWr->row) {
            y = geo.y() - currGeo.y();
            x = geo.x();
        }
        else if (wr->row > currWr->row) {
            y = geo.y()
                    + (winGeo.height()-currGeo.y())
                    - cellSize.height() + winGeo.y();
            x = geo.x();
        }
        else {  // same row
            if (wr->col < currWr->col) {
                y = geo.y();
                x = geo.x() - currGeo.x();
            }
            else {
                y = geo.y();
                x = geo.x()
                        + (winGeo.width()-currGeo.x())
                        - cellSize.width() + winGeo.x();
            }
        }
        return QPoint(x,y);
    }
    QSize calculateCellSize(const QRect & geo, int spacing)
    {
        return QSize(
                    /*(double) */(geo.width()-((colCount-1)*spacing)) / colCount,
                    /*(double) */(geo.height()-((rowCount-1)*spacing)) / rowCount
                    );
    }
    QRect calculateCellGeometry(const QRect & adjustedGeo, const QSize & cellSize,
                                int spacing, int row, int col,
                                int rowSpan, int colSpan)
    {
        return QRect(
                    (cellSize.width()*col)+(spacing*col)+adjustedGeo.x(),
                    (cellSize.height()*row)+(spacing*row)+adjustedGeo.y(),
                    (cellSize.width()*colSpan)+(spacing*(colSpan-1)),
                    (cellSize.height()*rowSpan)+(spacing*(rowSpan-1))
                    );
    }
};


QAnimatedGridLayout::QAnimatedGridLayout(QWidget *parent) :
    QLayout(parent)
{
    m_private = new QAnimatedGridLayoutPrivate(this);
    connect(this, SIGNAL(animationFinished()), SLOT(animationFinishedSlot()));
}

QAnimatedGridLayout::~QAnimatedGridLayout()
{
    if (m_private)
        delete m_private;
}

void QAnimatedGridLayout::addItem(QLayoutItem *item)
{
    Q_CHECK_PTR(item);
    m_private->addItem(item,0,0,1,1);
}

void QAnimatedGridLayout::setGeometry(const QRect &rect)
{
    if (m_private->dirty || rect != geometry()) {
        int lm, tm, rm, bm;
        getContentsMargins(&lm, &tm, &rm, &bm);
        m_private->setGeometry(rect.adjusted(lm, tm, -rm, -bm));
        QLayout::setGeometry(rect);
    }
}

QSize QAnimatedGridLayout::sizeHint() const
{
    return m_private->sizeHint();
}

QLayoutItem *QAnimatedGridLayout::itemAt(int index) const
{
    return m_private->itemAt(index);
}

QLayoutItem *QAnimatedGridLayout::takeAt(int index)
{
    return m_private->takeAt(index);
}

int QAnimatedGridLayout::count() const
{
    return m_private->list.count();
}

QSize QAnimatedGridLayout::minimumSize() const
{
    return m_private->minimumSize();
}

bool QAnimatedGridLayout::hasHeightForWidth()
{
    return false;
}

bool QAnimatedGridLayout::isEmpty()
{
    return m_private->list.isEmpty();
}

void QAnimatedGridLayout::invalidate()
{
    m_private->dirty = true;
    QLayout::invalidate();
}

void QAnimatedGridLayout::addWidget(QWidget *widget,
                                    int row, int column,
                                    int rowSpan, int colSpan)
{
    Q_CHECK_PTR(widget);
    m_private->addItem(new QWidgetItem(widget),row,column,rowSpan,colSpan);
}

int QAnimatedGridLayout::rowCount()
{
    return m_private->rowCount;
}

int QAnimatedGridLayout::columnCount()
{
    return m_private->colCount;
}

void QAnimatedGridLayout::setEasingCurve(const QEasingCurve &easing)
{
    m_private->easing = easing;
}

QEasingCurve QAnimatedGridLayout::easingCurve() const
{
    return m_private->easing;
}

void QAnimatedGridLayout::setAnimationDuration(int duration)
{
    m_private->duration = duration;
}

int QAnimatedGridLayout::animationDuration()
{
    return m_private->duration;
}

bool QAnimatedGridLayout::isZoomed()
{
    return m_private->zoomedIndex >= 0;
}

QWidget *QAnimatedGridLayout::zoomedWidget()
{
    return m_private->zoomedTo();
}

void QAnimatedGridLayout::setVisibleAreaWidget(QWidget *widget)
{
    Q_CHECK_PTR(widget);
    m_private->outerWidgetForRect = widget;
}

void QAnimatedGridLayout::zoomTo(QWidget *widget)
{
    Q_CHECK_PTR(widget);
    m_private->startZoomInAnimation(widget);
}

void QAnimatedGridLayout::showAll()
{
    m_private->startZoomOutAnimation();
}

void QAnimatedGridLayout::animationFinishedSlot()
{
    m_private->setAnimationFinished();
    update();
}



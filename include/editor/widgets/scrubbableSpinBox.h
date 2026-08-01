#ifndef ATLAS_SCRUBBABLESPINBOX_H
#define ATLAS_SCRUBBABLESPINBOX_H

#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QMouseEvent>
#include <QSpinBox>
#include <QTimer>

#include <cmath>

template <typename SpinBox> class ScrubbableSpinBoxBase : public SpinBox {
  public:
    explicit ScrubbableSpinBoxBase(QWidget *parent = nullptr)
        : SpinBox(parent) {
        this->lineEdit()->installEventFilter(this);
        this->lineEdit()->setCursor(Qt::SizeHorCursor);
        this->setAccelerated(true);
        this->setButtonSymbols(QAbstractSpinBox::NoButtons);
    }

  protected:
    bool eventFilter(QObject *watched, QEvent *event) override {
        if (watched != this->lineEdit())
            return SpinBox::eventFilter(watched, event);
        if (event->type() == QEvent::MouseButtonPress) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton) {
                scrubStartX = mouse->globalPosition().x();
                scrubStartValue = this->value();
                scrubbing = false;
                selectOnRelease = !this->lineEdit()->hasFocus();
            }
        } else if (event->type() == QEvent::MouseMove) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->buttons().testFlag(Qt::LeftButton)) {
                const double distance =
                    mouse->globalPosition().x() - scrubStartX;
                if (std::abs(distance) >= 3.0)
                    scrubbing = true;
                if (scrubbing) {
                    const double precision =
                        mouse->modifiers().testFlag(Qt::ShiftModifier) ? 0.1
                                                                       : 1.0;
                    this->setValue(scrubStartValue +
                                   distance * this->singleStep() * precision);
                    return true;
                }
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton && scrubbing) {
                scrubbing = false;
                selectOnRelease = false;
                return true;
            }
            if (mouse->button() == Qt::LeftButton &&
                (selectOnRelease || !this->lineEdit()->hasSelectedText())) {
                selectOnRelease = false;
                QTimer::singleShot(0, this->lineEdit(),
                                   [this] { this->lineEdit()->selectAll(); });
            }
        }
        return SpinBox::eventFilter(watched, event);
    }

  private:
    double scrubStartX = 0.0;
    double scrubStartValue = 0.0;
    bool scrubbing = false;
    bool selectOnRelease = false;
};

using ScrubbableDoubleSpinBox = ScrubbableSpinBoxBase<QDoubleSpinBox>;
using ScrubbableSpinBox = ScrubbableSpinBoxBase<QSpinBox>;

#endif

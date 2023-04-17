#include "hash/wheathash.hpp"

#include <QApplication>
#include <QElapsedTimer>
#include <QImage>
#include <QMainWindow>
#include <QPainter>
#include <QtMath>

#include <functional>
#include <memory>
#include <string.h>

struct VisualizationWidget final : public QWidget
{
	using QWidget::QWidget;

	void setRngFunction(std::function<uint32_t (uint32_t)> nextRandomNumber) {
		_nextRandomNumber = std::move(nextRandomNumber);
		update();
	}

protected:
	void paintEvent(QPaintEvent*) override
	{
		QElapsedTimer timer;
		timer.start();

		QPainter p{this};
		if (!_nextRandomNumber)
			return;

		const int w = qFloor(width() * devicePixelRatioF());
		const int h = qFloor(height() * devicePixelRatioF());
		std::unique_ptr<uint32_t[]> field(new uint32_t[w * h]);
		uint32_t* fieldPtr = field.get();
		::memset(fieldPtr, 0, w * h * sizeof(fieldPtr[0]));

		int x = 0, y = 0;
		for (size_t i = 0; i < w * h * 40; ++i)
		{
			const uint32_t direction = _nextRandomNumber(4);
			switch (direction)
			{
			case 0:
				++x;
				if (x >= w)
					x -= w;
				break;
			case 1:
				--x;
				if (x < 0)
					x += w;
				break;
			case 2:
				++y;
				if (y >= h)
					y -= h;
				break;
			case 3:
				--y;
				if (y < 0)
					y += h;
				break;
			}

			++fieldPtr[y * w + x];
		}

		// Post-process the values into image
		for (size_t i = 0; i < w * h; ++i)
		{
			QColor color;
			color.setHsl(270 + std::min(fieldPtr[i], 255u) * 360 / 256, 255, 200);
			fieldPtr[i] = color.rgb();
		}

		QImage canvas((const uchar*)fieldPtr, w, h, QImage::Format_RGB32);
		p.drawImage(0, 0, canvas);

		parentWidget()->setWindowTitle(QString::number(timer.elapsed()));
	}

private:
	std::function<uint32_t (uint32_t)> _nextRandomNumber;
};

static inline uint32_t fnv1a(uint32_t max)
{
	static uint64_t seed = 1000;
	const auto* byte = reinterpret_cast<const uint8_t*>(&seed);

	static constexpr uint32_t Prime = 0x01000193; //   16777619
	for (size_t i = 0; i < sizeof(seed); ++i)
	{
		seed *= Prime;
		seed ^= byte[i];
	}

	return seed % max;
}

static inline uint32_t wheathash(uint32_t max)
{
	static uint64_t seed = 1000;
	
	seed = wheathash64v(seed);
	return seed % max;
}

int main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	QMainWindow w;
	auto visualizer = new VisualizationWidget{&w};
	//visualizer->setRngFunction([](uint32_t maxValue) {
	//	return (uint32_t)rand() % maxValue;
	//});

	visualizer->setRngFunction(&wheathash);
	//visualizer->setRngFunction(&fnv1a);

	w.resize(1024, 768);
	w.setCentralWidget(visualizer);
	w.show();

	return app.exec();
}
